#include <app/SauceEngineApp.hpp>
#include <app/ui/components/HelloWorldWindow.hpp>
#include <app/ui/components/DebugStatsWindow.hpp>
#include <app/components/TransformComponent.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <functional>
#include <cstring>

namespace sauce {

SauceEngineApp::SauceEngineApp() {
  pImGuiComponentManager = std::make_unique<sauce::ui::ImGuiComponentManager>();
}

SauceEngineApp::~SauceEngineApp() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void SauceEngineApp::run() {
  initWindow();
  initVulkan();

  // Load scene file if specified
  if (!sceneFile.empty() && pScene) {
    if (pScene->loadFromFile(sceneFile) && !pScene->getEntities().empty()) {
      uploadMeshGPUResources();
      setupSceneRenderer();
    }
  }


  // Call custom UI builder after ImGui is initialized
  if (pCustomUIBuilder) {
    pCustomUIBuilder(*pImGuiComponentManager);
  }

  mainLoop();
}

void SauceEngineApp::setCustomUIBuilder(std::function<void(sauce::ui::ImGuiComponentManager&)> builder) {
  pCustomUIBuilder = std::move(builder);
}

void SauceEngineApp::initVulkan() {
    uint32_t glfwExtensionsCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
    pInstance = std::make_unique<sauce::Instance>(glfwExtensions, glfwExtensionsCount);

    pRenderSurface = std::make_unique<sauce::RenderSurface>(*pInstance, window);

    physicalDevice = { *pInstance };
    logicalDevice = { physicalDevice, *pRenderSurface };

    sauce::CameraCreateInfo cameraCreateInfo {
      .scrWidth = WIDTH,
      .scrHeight = HEIGHT,
    };

    pScene = std::make_unique<sauce::Scene>( cameraCreateInfo );

    sauce::RendererCreateInfo rendererCreateInfo {
      .physicalDevice = physicalDevice,
      .logicalDevice = logicalDevice,
      .renderSurface = *pRenderSurface,
      .window = window,
    };

    pRenderer = std::make_unique<sauce::Renderer>(rendererCreateInfo);

    // Initialize ImGui
    sauce::ImGuiRendererCreateInfo imguiCreateInfo{
      .instance = **pInstance,
      .physicalDevice = physicalDevice,
      .logicalDevice = logicalDevice,
      .queueFamilyIndex = logicalDevice.getQueueIndex(),
      .window = window,
      .queue = pRenderer->getQueue(),
      .commandPool = pRenderer->getCommandPool(),
      .swapChain = pRenderer->getSwapChain(),
      .imageCount = static_cast<uint32_t>(pRenderer->getSwapChain().getImages().size()),
      .swapChainFormat = pRenderer->getSwapChain().getSurfaceFormat().format,
      .depthFormat = sauce::GraphicsPipeline::findDepthFormat(physicalDevice),
    };

    pImGuiRenderer = std::make_unique<sauce::ImGuiRenderer>(imguiCreateInfo);

    // Add default UI components
    pImGuiComponentManager->addComponent(std::make_unique<sauce::ui::HelloWorldWindow>());
    pImGuiComponentManager->addComponent(std::make_unique<sauce::ui::DebugStatsWindow>());
  }

void SauceEngineApp::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Playground", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

void SauceEngineApp::processInput(float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS && !gravePressedLastFrame) {
      cursorCaptured = !cursorCaptured;
      glfwSetInputMode(window, GLFW_CURSOR, cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
      firstMouse = true;
    }
    gravePressedLastFrame = glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS;

    if (!cursorCaptured || !pScene) {
      return;
    }

    auto& camera = pScene->getCameraRW();

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      camera.processDirection(Camera::Movement::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      camera.processDirection(Camera::Movement::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      camera.processDirection(Camera::Movement::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      camera.processDirection(Camera::Movement::RIGHT, deltaTime);
  }

void SauceEngineApp::mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
    auto* app = static_cast<SauceEngineApp*>(glfwGetWindowUserPointer(window));
    if (!app || !app->cursorCaptured) {
      return;
    }

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (app->firstMouse) {
      app->lastX = xpos;
      app->lastY = ypos;
      app->firstMouse = false;
    }

    float xoffset = xpos - app->lastX;
    float yoffset = app->lastY - ypos;

    app->lastX = xpos;
    app->lastY = ypos;

    if (!app->pScene) {
      return;
    }

    app->pScene->getCameraRW().processMouseMovement(xoffset, yoffset);
  }

void SauceEngineApp::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      auto currentFrameTime = std::chrono::steady_clock::now();
      deltaTime = std::chrono::duration<float>(currentFrameTime - lastFrameTime).count();
      lastFrameTime = currentFrameTime;

      glfwPollEvents();
      processInput(deltaTime);

      pImGuiRenderer->newFrame();
      buildExampleUI();

      pRenderer->drawFrame(logicalDevice, *pScene, pImGuiRenderer.get());
    }

    logicalDevice->waitIdle();
  }

void SauceEngineApp::buildExampleUI() {
    pImGuiComponentManager->renderAll();
  }

void SauceEngineApp::uploadMeshGPUResources() {
  for (auto& entity : pScene->getEntitiesMut()) {
    auto mrcs = entity.getComponents<MeshRendererComponent>();
    for (auto* mrc : mrcs) {
      auto mesh = mrc->getMesh();
      if (!mesh || !mesh->isValid()) continue;
      if (!mesh->hasGPUData()) {
        auto& physDev = const_cast<vk::raii::PhysicalDevice&>(*physicalDevice);
        auto& cmdPool = const_cast<vk::raii::CommandPool&>(pRenderer->getCommandPool());
        auto& queue = const_cast<vk::raii::Queue&>(pRenderer->getQueue());
        mesh->initVulkanResources(logicalDevice, physDev, cmdPool, queue);
      }
    }
  }
}

void SauceEngineApp::setupSceneRenderer() {
  pRenderer->setCommandBufferRecorder(
    [this](vk::raii::CommandBuffer& cmd, uint32_t imageIndex) {
      recordSceneCommandBuffer(cmd, imageIndex);
    }
  );
}

void SauceEngineApp::recordSceneCommandBuffer(vk::raii::CommandBuffer& cmd, uint32_t imageIndex) {
  // Write camera matrices to UBO (host side, before GPU execution)
  sauce::UniformBufferObject ubo {
    .model = glm::mat4(1.0f),
    .view = pScene->getCameraRO().getViewMatrix(),
    .proj = pScene->getCameraRO().getProjectionMatrix(),
    .cameraPos = pScene->getCameraRO().getPos(),
  };
  ubo.proj[1][1] *= -1; // Vulkan Y-flip
  std::memcpy(pRenderer->getCurrentUniformBufferMapped(), &ubo, sizeof(ubo));

  cmd.begin({});

  // Transition swapchain image -> ColorAttachmentOptimal
  pRenderer->transitionImageLayout(
    cmd,
    pRenderer->getSwapChain().getImages()[imageIndex],
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eColorAttachmentOptimal,
    {},
    vk::AccessFlagBits2::eColorAttachmentWrite,
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::ImageAspectFlagBits::eColor
  );

  // Transition depth image -> DepthAttachmentOptimal
  pRenderer->transitionImageLayout(
    cmd,
    *pRenderer->getDepthImage(),
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eDepthAttachmentOptimal,
    vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
    vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
    vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
    vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
    vk::ImageAspectFlagBits::eDepth
  );

  auto& swapChain = pRenderer->getSwapChain();
  vk::Extent2D extent = swapChain.getExtent();

  vk::ClearValue clearColor = vk::ClearColorValue { 0.0f, 0.0f, 0.0f, 1.0f };
  vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

  bool firstDraw = true;

  for (auto& entity : pScene->getEntitiesMut()) {
    if (!entity.getActive()) continue;

    auto mrcs = entity.getComponents<MeshRendererComponent>();
    if (mrcs.empty()) continue;

    // Get model matrix from transform
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    auto* tc = entity.getComponent<TransformComponent>();
    if (tc) {
      modelMatrix = tc->getLocalMatrix();
    }

    // Update model matrix in UBO via vkCmdUpdateBuffer (GPU-side, outside render pass)
    cmd.updateBuffer<glm::mat4>(*pRenderer->getCurrentUniformBuffer(), 0, modelMatrix);

    // Barrier: transfer write -> vertex shader uniform read
    vk::BufferMemoryBarrier2 bufBarrier {
      .srcStageMask = vk::PipelineStageFlagBits2::eTransfer,
      .srcAccessMask = vk::AccessFlagBits2::eTransferWrite,
      .dstStageMask = vk::PipelineStageFlagBits2::eVertexShader,
      .dstAccessMask = vk::AccessFlagBits2::eUniformRead,
      .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
      .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
      .buffer = *pRenderer->getCurrentUniformBuffer(),
      .offset = 0,
      .size = sizeof(glm::mat4),
    };
    vk::DependencyInfo depInfo {
      .bufferMemoryBarrierCount = 1,
      .pBufferMemoryBarriers = &bufBarrier,
    };
    cmd.pipelineBarrier2(depInfo);

    // Begin rendering (clear on first, load on subsequent)
    vk::RenderingAttachmentInfo colorAttachment {
      .imageView = swapChain.getImageViews()[imageIndex],
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = firstDraw ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = clearColor,
    };
    vk::RenderingAttachmentInfo depthAttachment {
      .imageView = pRenderer->getDepthImageView(),
      .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
      .loadOp = firstDraw ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = clearDepth,
    };
    vk::RenderingInfo renderingInfo {
      .renderArea = { .offset = { 0, 0 }, .extent = extent },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
      .pDepthAttachment = &depthAttachment,
    };

    cmd.beginRendering(renderingInfo);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pRenderer->getPipeline());
    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f,
      static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f));
    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
      pRenderer->getPipeline().getLayout(), 0, *pRenderer->getCurrentDescriptorSet(), nullptr);

    for (auto* mrc : mrcs) {
      auto mesh = mrc->getMesh();
      if (!mesh || !mesh->hasGPUData()) continue;
      mesh->bind(cmd);
      mesh->draw(cmd);
    }

    cmd.endRendering();
    firstDraw = false;
  }

  // If no entities were drawn, still clear the screen
  if (firstDraw) {
    vk::RenderingAttachmentInfo colorAttachment {
      .imageView = swapChain.getImageViews()[imageIndex],
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eStore,
      .clearValue = clearColor,
    };
    vk::RenderingAttachmentInfo depthAttachment {
      .imageView = pRenderer->getDepthImageView(),
      .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eClear,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
      .clearValue = clearDepth,
    };
    vk::RenderingInfo renderingInfo {
      .renderArea = { .offset = { 0, 0 }, .extent = extent },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
      .pDepthAttachment = &depthAttachment,
    };
    cmd.beginRendering(renderingInfo);
    cmd.endRendering();
  }

  // ImGui pass (load existing framebuffer contents)
  {
    vk::RenderingAttachmentInfo colorAttachment {
      .imageView = swapChain.getImageViews()[imageIndex],
      .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eLoad,
      .storeOp = vk::AttachmentStoreOp::eStore,
    };
    vk::RenderingAttachmentInfo depthAttachment {
      .imageView = pRenderer->getDepthImageView(),
      .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
      .loadOp = vk::AttachmentLoadOp::eLoad,
      .storeOp = vk::AttachmentStoreOp::eDontCare,
    };
    vk::RenderingInfo renderingInfo {
      .renderArea = { .offset = { 0, 0 }, .extent = extent },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachment,
      .pDepthAttachment = &depthAttachment,
    };
    cmd.beginRendering(renderingInfo);
    if (pImGuiRenderer) {
      pImGuiRenderer->render(cmd, imageIndex);
    }
    cmd.endRendering();
  }

  // Transition to present
  pRenderer->transitionImageLayout(
    cmd,
    pRenderer->getSwapChain().getImages()[imageIndex],
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::ImageLayout::ePresentSrcKHR,
    vk::AccessFlagBits2::eColorAttachmentWrite,
    {},
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::PipelineStageFlagBits2::eBottomOfPipe,
    vk::ImageAspectFlagBits::eColor
  );

  cmd.end();
}

} // namespace sauce
