#include <app/SauceEngineApp.hpp>
#include <app/ui/components/HelloWorldWindow.hpp>
#include <app/ui/components/DebugStatsWindow.hpp>
#include <app/components/TransformComponent.hpp>
#include <app/components/RigidBodyComponent.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <app/components/ClothComponent.hpp>
#include <app/components/LightComponent.hpp>
#include <functional>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <limits>

#include <physics/XPBD.hpp>
#include <physics/constraints/Constraint.hpp>

namespace sauce {

namespace {

struct SceneBounds {
  glm::vec3 min{0.0f};
  glm::vec3 max{0.0f};
  bool valid = false;
};

SceneBounds computeSceneBounds(const Scene& scene) {
  SceneBounds bounds;

  for (const auto& entity : scene.getEntities()) {
    if (!entity.getActive()) {
      continue;
    }

    glm::mat4 modelMatrix(1.0f);
    if (const auto* transform = entity.getComponent<TransformComponent>()) {
      modelMatrix = transform->getLocalMatrix();
    }

    for (const auto* meshRenderer : entity.getComponents<MeshRendererComponent>()) {
      if (!meshRenderer) {
        continue;
      }

      const auto mesh = meshRenderer->getMesh();
      if (!mesh || mesh->getVertices().empty()) {
        continue;
      }

      for (const auto& vertex : mesh->getVertices()) {
        const glm::vec3 worldPos =
            glm::vec3(modelMatrix * glm::vec4(vertex.position, 1.0f));
        if (!bounds.valid) {
          bounds.min = worldPos;
          bounds.max = worldPos;
          bounds.valid = true;
          continue;
        }

        bounds.min = glm::min(bounds.min, worldPos);
        bounds.max = glm::max(bounds.max, worldPos);
      }
    }
  }

  return bounds;
}

} // namespace

SauceEngineApp::SauceEngineApp() {
  pImGuiComponentManager = std::make_unique<sauce::ui::ImGuiComponentManager>();
}

SauceEngineApp::~SauceEngineApp() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void SauceEngineApp::run(const uint32_t width, const uint32_t height) {
  this->width = width;
  this->height = height;
  initWindow();
  initVulkan();

  // Load scene file if specified
  if (!sceneFile.empty() && pScene) {
    if (pScene->loadFromFile(sceneFile) && !pScene->getEntities().empty()) {
      frameLoadedSceneCamera();
      uploadMeshGPUResources();
      setupSceneRenderer();
    }
  }

  // Load IBL if specified
  if (!iblFile.empty() && pRenderer) {
    pRenderer->loadIBL(iblFile);
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
      .scrWidth = static_cast<float>(width),
      .scrHeight = static_cast<float>(height),
    };

    pScene = std::make_unique<sauce::Scene>( cameraCreateInfo );

    sauce::RendererCreateInfo rendererCreateInfo {
      .physicalDevice = physicalDevice,
      .logicalDevice = logicalDevice,
      .renderSurface = *pRenderSurface,
      .window = window,
    };

    pRenderer = std::make_unique<sauce::Renderer>(rendererCreateInfo);

    pSolver = std::make_unique<physics::XPBDSolver>();

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

    window = glfwCreateWindow(width, height, "Vulkan Playground", nullptr, nullptr);

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

    const bool spacePressed = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (spacePressed && !spacePressedLastFrame) {
      applyClothImpulse();
    }
    spacePressedLastFrame = spacePressed;

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

void SauceEngineApp::applyClothImpulse() {
    if (!pScene) {
      return;
    }

    const Camera& camera = pScene->getCameraRO();
    const glm::vec3 impulseDirection = glm::normalize(camera.getFront());
    constexpr float kImpulseStrength = 2.0f;

    for (auto& entity : pScene->getEntitiesMut()) {
      if (!entity.getActive()) {
        continue;
      }

      for (auto* clothComp : entity.getComponents<ClothComponent>()) {
        physics::ClothData* cloth = clothComp->getClothData();
        if (!cloth || cloth->particles.empty()) {
          continue;
        }

        glm::vec3 center(0.0f);
        glm::vec3 minPos = cloth->particles.front().position;
        glm::vec3 maxPos = cloth->particles.front().position;
        size_t dynamicCount = 0;

        for (const auto& particle : cloth->particles) {
          center += particle.position;
          minPos = glm::min(minPos, particle.position);
          maxPos = glm::max(maxPos, particle.position);
          if (!particle.isStatic()) {
            ++dynamicCount;
          }
        }

        if (dynamicCount == 0) {
          continue;
        }

        center /= static_cast<float>(cloth->particles.size());

        const float clothRadius =
            std::max(0.25f, 0.35f * glm::length(maxPos - minPos));

        for (auto& particle : cloth->particles) {
          if (particle.isStatic()) {
            continue;
          }

          const float distance = glm::length(particle.position - center);
          if (distance > clothRadius) {
            continue;
          }

          const float falloff = 1.0f - (distance / clothRadius);
          const glm::vec3 deltaVelocity =
              impulseDirection * (kImpulseStrength * falloff);

          particle.velocity += deltaVelocity;
          particle.predictedPosition += deltaVelocity * (1.0f / 128.0f);
        }
      }
    }
  }

void SauceEngineApp::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      auto currentFrameTime = std::chrono::steady_clock::now();
      deltaFrame = std::chrono::duration<float>(currentFrameTime - lastFrameTime).count();
      lastFrameTime = currentFrameTime;
      deltaUpdate += deltaFrame;

      glfwPollEvents();
      processInput(deltaFrame);
      syncRigidBodiesToTransforms();

      pImGuiRenderer->newFrame();
      buildExampleUI();

      // Run XPBD only if 1/TICKRATE seconds passed since last physics run 

      auto rigidBodies = std::vector<RigidBodyComponent>();
      auto constraints = std::vector<std::unique_ptr<physics::Constraint>>();

      for (auto& entity: pScene->getEntities()) {
        auto rigidBody = entity.getComponent<RigidBodyComponent>();
        if (rigidBody) rigidBodies.push_back(*rigidBody);
      }

      constexpr float kPhysicsDt = 1.0f / 128.0f;
      constexpr int kMaxPhysicsStepsPerFrame = 2;
      constexpr float kMaxPhysicsAccumulation =
          kPhysicsDt * static_cast<float>(kMaxPhysicsStepsPerFrame);
      auto& clothPhysicalDevice =
          const_cast<vk::raii::PhysicalDevice&>(*physicalDevice);
      auto& clothCommandPool =
          const_cast<vk::raii::CommandPool&>(pRenderer->getCommandPool());
      auto& clothQueue =
          const_cast<vk::raii::Queue&>(pRenderer->getQueue());

      if (deltaUpdate > kMaxPhysicsAccumulation) {
        deltaUpdate = kMaxPhysicsAccumulation;
      }

      int physicsStepsThisFrame = 0;
      while (deltaUpdate >= kPhysicsDt &&
             physicsStepsThisFrame < kMaxPhysicsStepsPerFrame) {
        pSolver->solvePositions(rigidBodies, constraints, kPhysicsDt);
        for (auto& entity : pScene->getEntitiesMut()) {
          if (!entity.getActive()) {
            continue;
          }

          for (auto* clothComp : entity.getComponents<ClothComponent>()) {
            clothComp->syncSimulationTransform();

            physics::ClothData* cloth = clothComp->getClothData();
            if (cloth && !cloth->empty()) {
              pSolver->solveCloth(
                *cloth,
                clothComp->getSettings(),
                kPhysicsDt);
              clothComp->markRuntimeMeshDirty();
            }
          }
        }
        deltaUpdate -= kPhysicsDt;
        ++physicsStepsThisFrame;
      }

      for (auto& entity : pScene->getEntitiesMut()) {
        if (!entity.getActive()) {
          continue;
        }

        for (auto* clothComp : entity.getComponents<ClothComponent>()) {
          auto runtimeMesh = clothComp->getRuntimeMesh();
          if (!runtimeMesh) {
            continue;
          }

          MeshRendererComponent* clothRenderer = nullptr;
          auto meshRenderers = entity.getComponents<MeshRendererComponent>();
          for (auto* meshRenderer : meshRenderers) {
            if (meshRenderer && meshRenderer->getMesh() == runtimeMesh) {
              clothRenderer = meshRenderer;
              break;
            }
          }

          if (!clothRenderer && !meshRenderers.empty()) {
            clothRenderer = meshRenderers.front();
            if (clothRenderer->getMesh() != runtimeMesh) {
              clothRenderer->setMesh(runtimeMesh);
            }
          }

          bool regenerateTangents = true;
          if (clothRenderer) {
            const auto material = clothRenderer->getMaterial();
            regenerateTangents =
                material && material->getTexture(modeling::TextureType::Normal);
          }

          if (clothComp->isRuntimeMeshDirty() &&
              !clothComp->syncRuntimeMesh(regenerateTangents)) {
            continue;
          }

          if (!runtimeMesh->isValid()) {
            continue;
          }

          if (!runtimeMesh->hasGPUData()) {
            runtimeMesh->initVulkanResources(
              logicalDevice,
              clothPhysicalDevice,
              clothCommandPool,
              clothQueue);
          } else {
            runtimeMesh->updateVertexBuffer(
              logicalDevice,
              clothPhysicalDevice,
              clothCommandPool,
              clothQueue);
          }
        }
      }

      pRenderer->drawFrame(logicalDevice, *pScene, pImGuiRenderer.get());
    }

    logicalDevice->waitIdle();
  }

void SauceEngineApp::buildExampleUI() {
    pImGuiComponentManager->renderAll();
  }

void SauceEngineApp::syncRigidBodiesToTransforms() {
    if (!pScene) {
      return;
    }

    for (auto& entity : pScene->getEntitiesMut()) {
      auto* rigidBody = entity.getComponent<RigidBodyComponent>();
      auto* transform = entity.getComponent<TransformComponent>();
      if (!rigidBody || !transform) {
        continue;
      }

      transform->setTranslation(rigidBody->getPosition());
      transform->setRotation(rigidBody->getOrientation());
    }
  }

void SauceEngineApp::uploadMeshGPUResources() {
  for (auto& entity : pScene->getEntitiesMut()) {
    auto mrcs = entity.getComponents<MeshRendererComponent>();
    for (auto* mrc : mrcs) {
      auto mesh = mrc->getMesh();
      if (mesh && mesh->isValid() && !mesh->hasGPUData()) {
        auto& physDev = const_cast<vk::raii::PhysicalDevice&>(*physicalDevice);
        auto& cmdPool = const_cast<vk::raii::CommandPool&>(pRenderer->getCommandPool());
        auto& queue = const_cast<vk::raii::Queue&>(pRenderer->getQueue());
        mesh->initVulkanResources(logicalDevice, physDev, cmdPool, queue);
      }

      auto material = mrc->getMaterial();
      if (material && !material->hasDescriptorSet()) {
        auto& physDev = const_cast<vk::raii::PhysicalDevice&>(*physicalDevice);
        auto& cmdPool = const_cast<vk::raii::CommandPool&>(pRenderer->getCommandPool());
        auto& queue = const_cast<vk::raii::Queue&>(pRenderer->getQueue());
        material->initVulkanResources(
          logicalDevice, physDev, cmdPool, queue,
          pRenderer->getDescriptorPool(),
          pRenderer->getDefaultImageView(),
          pRenderer->getDefaultSampler()
        );
      }
    }
  }
}

void SauceEngineApp::frameLoadedSceneCamera() {
  if (!pScene) {
    return;
  }

  const SceneBounds bounds = computeSceneBounds(*pScene);
  if (!bounds.valid) {
    return;
  }

  const glm::vec3 center = 0.5f * (bounds.min + bounds.max);
  const glm::vec3 extents = 0.5f * (bounds.max - bounds.min);
  const float radius = std::max(glm::length(extents), 0.5f);

  auto& camera = pScene->getCameraRW();
  const float halfFovRadians = glm::radians(camera.getFOV() * 0.5f);
  const float minDistance = radius * 2.0f;
  const float fitDistance = radius / std::max(std::tan(halfFovRadians), 0.1f);
  const float distance = std::max(minDistance, fitDistance * 1.2f);

  const glm::vec3 viewDirection = glm::normalize(glm::vec3(1.0f, 1.0f, 0.65f));
  camera.lookAt(center + viewDirection * distance, center, glm::vec3(0.0f, 0.0f, 1.0f));
  firstMouse = true;
}

void SauceEngineApp::setupSceneRenderer() {
  pRenderer->setCommandBufferRecorder(
    [this](vk::raii::CommandBuffer& cmd, uint32_t imageIndex) {
      recordSceneCommandBuffer(cmd, imageIndex);
    }
  );
}

struct ScenePushConstants {
  glm::mat4 model;
  uint32_t lightCount;
};

void SauceEngineApp::setupXPBDSolver() {
  
}

void SauceEngineApp::recordSceneCommandBuffer(vk::raii::CommandBuffer& cmd, uint32_t imageIndex) {
  // Write camera matrices to UBO (host side, before GPU execution)
  sauce::UniformBufferObject uboData {
    .model = glm::mat4(1.0f),
    .view = pScene->getCameraRO().getViewMatrix(),
    .proj = pScene->getCameraRO().getProjectionMatrix(),
    .cameraPos = pScene->getCameraRO().getPos(),
  };
  uboData.proj[1][1] *= -1; // Vulkan Y-flip
  std::memcpy(pRenderer->getCurrentUniformBufferMapped(), &uboData, sizeof(uboData));

  auto gpuLights = pScene->collectGPULights();
  if (gpuLights.empty()) {
    GPULight keyLight{};
    keyLight.type = static_cast<uint32_t>(LightType::Directional);
    keyLight.direction = glm::normalize(glm::vec3(1.0f, -1.0f, -1.0f));
    keyLight.color = glm::vec3(1.0f);
    keyLight.intensity = 2.5f;
    gpuLights.push_back(keyLight);

    GPULight fillLight{};
    fillLight.type = static_cast<uint32_t>(LightType::Directional);
    fillLight.direction = -keyLight.direction;
    fillLight.color = glm::vec3(0.55f, 0.6f, 0.7f);
    fillLight.intensity = 1.25f;
    gpuLights.push_back(fillLight);
  }

  uint32_t lightCount = pRenderer->updateLightSSBO(
      gpuLights.data(), static_cast<uint32_t>(gpuLights.size()));

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

  // Pass 0: Clear and Render Skybox
  {
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
    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f,
      static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f));
    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));
    
    pRenderer->renderEnvironmentMap(cmd);
    
    cmd.endRendering();
  }

  bool firstDraw = false; // We already cleared in Pass 0

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

    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f,
      static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f));
    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));

    for (auto* mrc : mrcs) {
      auto mesh = mrc->getMesh();
      auto material = mrc->getMaterial();
      if (!mesh || !mesh->hasGPUData()) continue;

      const auto& pipeline =
        (material && material->getProperties().doubleSided)
          ? pRenderer->getDoubleSidedPipeline()
          : pRenderer->getPipeline();

      cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);

      // Bind Set 0: Per-frame
      cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
        pipeline.getLayout(), 0, { *pRenderer->getCurrentDescriptorSet() }, nullptr);

      // Bind Set 1: Environment
      cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
        pipeline.getLayout(), 1, { pRenderer->getEnvironmentDescriptorSet() }, nullptr);

      ScenePushConstants pushData {
        .model = modelMatrix,
        .lightCount = lightCount
      };
      cmd.pushConstants<ScenePushConstants>(
        *pipeline.getLayout(),
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        0u, pushData
      );

      mesh->bind(cmd);

      // Bind Set 2: Material
      if (material && material->hasDescriptorSet()) {
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
          pipeline.getLayout(), 2, { material->getDescriptorSet() }, nullptr);
      } else {
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
          pipeline.getLayout(), 2, { pRenderer->getDefaultMaterialDescriptorSet() }, nullptr);
      }
      
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
