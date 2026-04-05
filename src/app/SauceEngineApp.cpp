#include <app/SauceEngineApp.hpp>
#include <app/ui/components/HelloWorldWindow.hpp>
#include <app/ui/components/DebugStatsWindow.hpp>
#include <app/components/TransformComponent.hpp>
#include <app/components/RigidBodyComponent.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <app/components/LightComponent.hpp>

#include <imgui_stdlib.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <sstream>

#include <glm/gtc/quaternion.hpp>

namespace sauce {

namespace {

constexpr const char* kLauncherWindowTitle = "SauceEngine Launcher";
constexpr const char* kEngineWindowTitle = "SauceEngine";

std::string lowercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::array<float, 3> defaultModelRotationDegrees() {
  return {
      static_cast<float>(AppOptions::DEFAULT_MODEL_ROTATE_X_DEGREES),
      static_cast<float>(AppOptions::DEFAULT_MODEL_ROTATE_Y_DEGREES),
      static_cast<float>(AppOptions::DEFAULT_MODEL_ROTATE_Z_DEGREES)};
}

bool pathLooksLikeAuthoredScene(const std::string& scenePath) {
  if (scenePath.empty()) {
    return false;
  }

  const std::string normalizedPath = lowercase(std::filesystem::path(scenePath).generic_string());
  return normalizedPath.find("/testscene/") != std::string::npos ||
         normalizedPath.ends_with("/testscene.gltf") ||
         normalizedPath.ends_with("/testscene.glb");
}

std::array<float, 3> resolvedModelRotationDegrees(
    const std::string& scenePath,
    bool explicitOverride,
    const std::array<float, 3>& configuredDegrees) {
  if (explicitOverride) {
    return configuredDegrees;
  }
  return pathLooksLikeAuthoredScene(scenePath) ? std::array<float, 3>{0.0f, 0.0f, 0.0f} : configuredDegrees;
}

void applyGlobalModelRotation(Scene& scene, const std::array<float, 3>& rotationDegrees) {
  if (std::abs(rotationDegrees[0]) <= 0.001f &&
      std::abs(rotationDegrees[1]) <= 0.001f &&
      std::abs(rotationDegrees[2]) <= 0.001f) {
    return;
  }

  const glm::quat rotateX = glm::angleAxis(glm::radians(rotationDegrees[0]), glm::vec3(1.0f, 0.0f, 0.0f));
  const glm::quat rotateY = glm::angleAxis(glm::radians(rotationDegrees[1]), glm::vec3(0.0f, 1.0f, 0.0f));
  const glm::quat rotateZ = glm::angleAxis(glm::radians(rotationDegrees[2]), glm::vec3(0.0f, 0.0f, 1.0f));
  const glm::quat rotation = glm::normalize(rotateZ * rotateY * rotateX);
  for (auto& entity : scene.getEntitiesMut()) {
    if (auto* transform = entity.getComponent<TransformComponent>()) {
      transform->setTranslation(rotation * transform->getTranslation());
      transform->setRotation(rotation * transform->getRotation());
    }

    if (auto* rigidBody = entity.getComponent<RigidBodyComponent>()) {
      rigidBody->setPosition(rotation * rigidBody->getPosition());
      rigidBody->setOrientation(rotation * rigidBody->getOrientation());
      rigidBody->setVelocity(rotation * rigidBody->getVelocity());
      rigidBody->setAngularVelocity(rotation * rigidBody->getAngularVelocity());
    }
  }
}
} // namespace

SauceEngineApp::SauceEngineApp() {
  pImGuiComponentManager = std::make_unique<sauce::ui::ImGuiComponentManager>();
  pLauncher = std::make_unique<sauce::launcher::AppLauncher>();
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
  setupSceneRenderer();

  if (launcherActive) {
    pLauncher->initialize(
        width,
        height,
        sceneFile,
        iblFile,
        polyHavenModelId,
        polyHavenModelResolution,
        polyHavenHdriId,
        polyHavenHdriResolution);
  } else {
    std::string remoteError;
    if (!resolveConfiguredRemoteAssets(remoteError)) {
      throw std::runtime_error(remoteError);
    }
    loadConfiguredScene();
  }

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
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
  pInstance = std::make_unique<sauce::Instance>(glfwExtensions, glfwExtensionsCount);

  pRenderSurface = std::make_unique<sauce::RenderSurface>(*pInstance, window);

  physicalDevice = {*pInstance};
  logicalDevice = {physicalDevice, *pRenderSurface};

  sauce::CameraCreateInfo cameraCreateInfo{
      .scrWidth = static_cast<float>(width),
      .scrHeight = static_cast<float>(height),
  };

  pScene = std::make_unique<sauce::Scene>(cameraCreateInfo);

  sauce::RendererCreateInfo rendererCreateInfo{
      .physicalDevice = physicalDevice,
      .logicalDevice = logicalDevice,
      .renderSurface = *pRenderSurface,
      .window = window,
  };

  pRenderer = std::make_unique<sauce::Renderer>(rendererCreateInfo);

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

  pImGuiComponentManager->addComponent(std::make_unique<sauce::ui::HelloWorldWindow>());
  pImGuiComponentManager->addComponent(std::make_unique<sauce::ui::DebugStatsWindow>());
}

void SauceEngineApp::initWindow() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window = glfwCreateWindow(
      static_cast<int>(width),
      static_cast<int>(height),
      launcherActive ? kLauncherWindowTitle : kEngineWindowTitle,
      nullptr,
      nullptr);

  glfwSetWindowUserPointer(window, this);
  glfwSetCursorPosCallback(window, mouseCallback);
  setCursorCapture(!launcherActive);
}

void SauceEngineApp::processInput(float deltaTime) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, true);
  }

  if (launcherActive) {
    return;
  }

  if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS && !gravePressedLastFrame) {
    setCursorCapture(!cursorCaptured);
  }
  gravePressedLastFrame = glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS;

  if (!cursorCaptured || !pScene) {
    return;
  }

  auto& camera = pScene->getCameraRW();

  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
    camera.processDirection(Camera::Movement::FORWARD, deltaTime);
  }
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
    camera.processDirection(Camera::Movement::BACKWARD, deltaTime);
  }
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
    camera.processDirection(Camera::Movement::LEFT, deltaTime);
  }
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
    camera.processDirection(Camera::Movement::RIGHT, deltaTime);
  }
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

    if (!launcherActive) {
      syncRigidBodiesToTransforms();
    } else {
      pLauncher->pollBackgroundTasks([this](const sauce::launcher::LaunchRequest& request) {
        return finalizeLauncherLaunch(request);
      });
    }

    pImGuiRenderer->newFrame();
    buildExampleUI();

    pRenderer->drawFrame(logicalDevice, *pScene, pImGuiRenderer.get());
  }

  logicalDevice->waitIdle();
}

void SauceEngineApp::buildExampleUI() {
  if (launcherActive) {
    pLauncher->render(
        [this](const sauce::launcher::LaunchRequest& request) {
          return finalizeLauncherLaunch(request);
        },
        [this]() {
          glfwSetWindowShouldClose(window, GLFW_TRUE);
        });
    return;
  }

  pImGuiComponentManager->renderAll();
}


bool SauceEngineApp::finalizeLauncherLaunch(const sauce::launcher::LaunchRequest& request) {
  const bool resolutionChanged = request.width != width || request.height != height;

  width = request.width;
  height = request.height;
  modelRotationDegrees = request.modelRotationDegrees;
  modelRotationExplicit = true;
  sceneFile = request.scenePath;
  iblFile = request.iblPath;

  if (!loadConfiguredScene()) {
    return false;
  }

  if (resolutionChanged) {
    glfwSetWindowSize(window, static_cast<int>(width), static_cast<int>(height));
    if (pScene) {
      pScene->getCameraRW().setScreenSize(static_cast<float>(width), static_cast<float>(height));
    }
    if (pRenderer) {
      pRenderer->recreateSwapChain();
    }
  }

  launcherActive = false;
  launcherEnabled = false;
  glfwSetWindowTitle(window, kEngineWindowTitle);
  setCursorCapture(true);
  return true;
}

bool SauceEngineApp::resolveConfiguredRemoteAssets(std::string& errorMessage) {
  if (!polyHavenModelId.empty()) {
    const auto modelDownload = sauce::launcher::downloadPolyHavenModelGltf(polyHavenModelId, polyHavenModelResolution);
    if (!modelDownload.errorMessage.empty()) {
      errorMessage = "Poly Haven model download failed: " + modelDownload.errorMessage;
      return false;
    }
    sceneFile = modelDownload.localPath.string();
    if (!modelRotationExplicit) {
      modelRotationDegrees = defaultModelRotationDegrees();
    }
  }

  if (!polyHavenHdriId.empty()) {
    const auto hdriDownload = sauce::launcher::downloadPolyHavenHdri(polyHavenHdriId, polyHavenHdriResolution);
    if (!hdriDownload.errorMessage.empty()) {
      errorMessage = "Poly Haven HDRI download failed: " + hdriDownload.errorMessage;
      return false;
    }
    iblFile = hdriDownload.localPath.string();
  }

  return true;
}

bool SauceEngineApp::loadConfiguredScene() {
  if (!pScene) {
    return false;
  }

  pScene->getEntitiesMut().clear();
  pScene->setCurrentFilePath("");
  pScene->getCameraRW().setScreenSize(static_cast<float>(width), static_cast<float>(height));

  if (!sceneFile.empty()) {
    if (!pScene->loadFromFile(sceneFile)) {
      return false;
    }
    if (!pScene->getEntities().empty()) {
      applyGlobalModelRotation(
          *pScene,
          resolvedModelRotationDegrees(sceneFile, modelRotationExplicit, modelRotationDegrees));
      uploadMeshGPUResources();
    }
  }

  if (!iblFile.empty() && pRenderer) {
    pRenderer->loadIBL(iblFile);
  }

  return true;
}

void SauceEngineApp::setCursorCapture(bool captured) {
  cursorCaptured = captured;
  glfwSetInputMode(window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
  firstMouse = true;
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
            logicalDevice,
            physDev,
            cmdPool,
            queue,
            pRenderer->getDescriptorPool(),
            pRenderer->getDefaultImageView(),
            pRenderer->getDefaultSampler());
      }
    }
  }
}

void SauceEngineApp::setupSceneRenderer() {
  pRenderer->setCommandBufferRecorder(
      [this](vk::raii::CommandBuffer& cmd, uint32_t imageIndex) {
        recordSceneCommandBuffer(cmd, imageIndex);
      });
}

struct ScenePushConstants {
  glm::mat4 model;
  uint32_t lightCount;
};

void SauceEngineApp::recordSceneCommandBuffer(vk::raii::CommandBuffer& cmd, uint32_t imageIndex) {
  sauce::UniformBufferObject uboData{
      .model = glm::mat4(1.0f),
      .view = pScene->getCameraRO().getViewMatrix(),
      .proj = pScene->getCameraRO().getProjectionMatrix(),
      .cameraPos = pScene->getCameraRO().getPos(),
  };
  uboData.proj[1][1] *= -1;
  std::memcpy(pRenderer->getCurrentUniformBufferMapped(), &uboData, sizeof(uboData));

  auto gpuLights = pScene->collectGPULights();

  uint32_t lightCount = pRenderer->updateLightSSBO(
      gpuLights.data(), static_cast<uint32_t>(gpuLights.size()));

  cmd.begin({});

  pRenderer->transitionImageLayout(
      cmd,
      pRenderer->getSwapChain().getImages()[imageIndex],
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::eColorAttachmentOptimal,
      {},
      vk::AccessFlagBits2::eColorAttachmentWrite,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::ImageAspectFlagBits::eColor);

  pRenderer->transitionImageLayout(
      cmd,
      *pRenderer->getDepthImage(),
      vk::ImageLayout::eUndefined,
      vk::ImageLayout::eDepthAttachmentOptimal,
      vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
      vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
      vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
      vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
      vk::ImageAspectFlagBits::eDepth);

  auto& swapChain = pRenderer->getSwapChain();
  vk::Extent2D extent = swapChain.getExtent();

  vk::ClearValue clearColor = vk::ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f};
  vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

  {
    vk::RenderingAttachmentInfo colorAttachment{
        .imageView = swapChain.getImageViews()[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor,
    };
    vk::RenderingAttachmentInfo depthAttachment{
        .imageView = pRenderer->getDepthImageView(),
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearDepth,
    };
    vk::RenderingInfo renderingInfo{
        .renderArea = {.offset = {0, 0}, .extent = extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = &depthAttachment,
    };
    cmd.beginRendering(renderingInfo);
    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f));
    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));

    pRenderer->renderEnvironmentMap(cmd);

    cmd.endRendering();
  }

  for (auto& entity : pScene->getEntitiesMut()) {
    if (!entity.getActive()) {
      continue;
    }

    auto mrcs = entity.getComponents<MeshRendererComponent>();
    if (mrcs.empty()) {
      continue;
    }

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    auto* tc = entity.getComponent<TransformComponent>();
    if (tc) {
      modelMatrix = tc->getLocalMatrix();
    }

    vk::RenderingAttachmentInfo colorAttachment{
        .imageView = swapChain.getImageViews()[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor,
    };
    vk::RenderingAttachmentInfo depthAttachment{
        .imageView = pRenderer->getDepthImageView(),
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearDepth,
    };
    vk::RenderingInfo renderingInfo{
        .renderArea = {.offset = {0, 0}, .extent = extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = &depthAttachment,
    };

    cmd.beginRendering(renderingInfo);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *pRenderer->getPipeline());
    cmd.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.0f, 1.0f));
    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));

    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        pRenderer->getPipeline().getLayout(),
        0,
        {*pRenderer->getCurrentDescriptorSet()},
        nullptr);

    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        pRenderer->getPipeline().getLayout(),
        1,
        {pRenderer->getEnvironmentDescriptorSet()},
        nullptr);

    ScenePushConstants pushData{
        .model = modelMatrix,
        .lightCount = lightCount,
    };
    cmd.pushConstants<ScenePushConstants>(
        *pRenderer->getPipeline().getLayout(),
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        0u,
        pushData);

    for (auto* mrc : mrcs) {
      auto mesh = mrc->getMesh();
      auto material = mrc->getMaterial();
      if (!mesh || !mesh->hasGPUData()) {
        continue;
      }
      mesh->bind(cmd);

      if (material && material->hasDescriptorSet()) {
        cmd.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            pRenderer->getPipeline().getLayout(),
            2,
            {material->getDescriptorSet()},
            nullptr);
      } else {
        cmd.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            pRenderer->getPipeline().getLayout(),
            2,
            {pRenderer->getDefaultMaterialDescriptorSet()},
            nullptr);
      }

      mesh->draw(cmd);
    }

    cmd.endRendering();
  }

  {
    vk::RenderingAttachmentInfo colorAttachment{
        .imageView = swapChain.getImageViews()[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
    };
    vk::RenderingAttachmentInfo depthAttachment{
        .imageView = pRenderer->getDepthImageView(),
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
    };
    vk::RenderingInfo renderingInfo{
        .renderArea = {.offset = {0, 0}, .extent = extent},
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

  pRenderer->transitionImageLayout(
      cmd,
      pRenderer->getSwapChain().getImages()[imageIndex],
      vk::ImageLayout::eColorAttachmentOptimal,
      vk::ImageLayout::ePresentSrcKHR,
      vk::AccessFlagBits2::eColorAttachmentWrite,
      {},
      vk::PipelineStageFlagBits2::eColorAttachmentOutput,
      vk::PipelineStageFlagBits2::eBottomOfPipe,
      vk::ImageAspectFlagBits::eColor);

  cmd.end();
}

} // namespace sauce
