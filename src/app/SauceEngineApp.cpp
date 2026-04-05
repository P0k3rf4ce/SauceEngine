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
#include <cstring>
#include <filesystem>
#include <functional>

namespace sauce {

namespace {

namespace fs = std::filesystem;

constexpr const char* kLauncherWindowTitle = "SauceEngine Launcher";
constexpr const char* kEngineWindowTitle = "SauceEngine";
constexpr const char* kDefaultLauncherScene = "assets/models/Cube.gltf";

struct LauncherResolutionOption {
  const char* label;
  uint32_t width;
  uint32_t height;
};

constexpr std::array<LauncherResolutionOption, 6> kLauncherResolutionOptions{{
    {"1280 x 720", 1280, 720},
    {"1600 x 900", 1600, 900},
    {"1920 x 1080", 1920, 1080},
    {"2560 x 1440", 2560, 1440},
    {"3024 x 1964", 3024, 1964},
    {"3840 x 2160", 3840, 2160},
}};

int findResolutionPreset(uint32_t width, uint32_t height) {
  for (size_t i = 0; i < kLauncherResolutionOptions.size(); ++i) {
    const auto& preset = kLauncherResolutionOptions[i];
    if (preset.width == width && preset.height == height) {
      return static_cast<int>(i);
    }
  }
  return -1;
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
  setupSceneRenderer();

  if (launcherActive) {
    launcherState.catalog = sauce::launcher::discoverAssetCatalog(fs::current_path());
    launcherState.selectedLaunchTarget = 0;
    launcherState.selectedIblMap = 0;
    launcherState.launchWidth = width;
    launcherState.launchHeight = height;
    launcherState.selectedResolutionPreset = findResolutionPreset(width, height);
    launcherState.scenePath = launcherState.catalog.launchTargets.empty()
        ? std::string()
        : launcherState.catalog.launchTargets.front().path;
    launcherState.iblPath = launcherState.catalog.iblMaps.empty()
        ? std::string()
        : launcherState.catalog.iblMaps.front().path;
    launcherState.statusMessage = "Select a scene, environment, and launch size.";
    updateLauncherPreview();
  } else {
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
    }

    pImGuiRenderer->newFrame();
    buildExampleUI();

    pRenderer->drawFrame(logicalDevice, *pScene, pImGuiRenderer.get());
  }

  logicalDevice->waitIdle();
}

void SauceEngineApp::buildExampleUI() {
  if (launcherActive) {
    renderLauncherUI();
    return;
  }

  pImGuiComponentManager->renderAll();
}

void SauceEngineApp::renderLauncherUI() {
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImDrawList* drawList = ImGui::GetBackgroundDrawList(viewport);
  const ImVec2 min = viewport->WorkPos;
  const ImVec2 max = ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y);

  drawList->AddRectFilled(min, max, IM_COL32(12, 15, 22, 255));
  drawList->AddRectFilledMultiColor(
      min,
      max,
      IM_COL32(25, 44, 92, 255),
      IM_COL32(12, 15, 22, 255),
      IM_COL32(12, 15, 22, 255),
      IM_COL32(20, 78, 66, 255));
  drawList->AddCircleFilled(ImVec2(max.x - 120.0f, min.y + 100.0f), 180.0f, IM_COL32(54, 96, 190, 55), 64);
  drawList->AddCircleFilled(ImVec2(min.x + 160.0f, max.y - 90.0f), 120.0f, IM_COL32(46, 164, 129, 45), 64);

  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(32.0f, 24.0f));

  const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoSavedSettings;
  ImGui::Begin("SauceLauncher", nullptr, flags);

  ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(233, 239, 247, 255));
  ImGui::TextUnformatted("SAUCE ENGINE");
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
  ImGui::SetWindowFontScale(1.45f);
  ImGui::TextUnformatted("Launcher");
  ImGui::SetWindowFontScale(1.0f);
  ImGui::TextColored(
      ImVec4(0.62f, 0.70f, 0.78f, 1.0f),
      "%zu targets  |  %zu maps  |  %u x %u",
      launcherState.catalog.launchTargets.size(),
      launcherState.catalog.iblMaps.size(),
      launcherState.launchWidth,
      launcherState.launchHeight);
  ImGui::Spacing();

  const float fullWidth = ImGui::GetContentRegionAvail().x;
  const float leftWidth = std::max(360.0f, fullWidth * 0.48f);
  const float rightWidth = std::max(320.0f, fullWidth - leftWidth - 20.0f);
  const float cardHeight = ImGui::GetContentRegionAvail().y - 84.0f;

  auto beginCard = [](const char* id, const ImVec2& size) {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(18, 24, 33, 215));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(59, 74, 96, 200));
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 18.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18.0f, 18.0f));
    ImGui::BeginChild(id, size, ImGuiChildFlags_Borders);
  };
  auto endCard = []() {
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
  };

  beginCard("LaunchTargetsCard", ImVec2(leftWidth, cardHeight));
  ImGui::TextUnformatted("Scenes and models");
  ImGui::TextColored(ImVec4(0.52f, 0.76f, 0.97f, 1.0f), "%zu available launch targets", launcherState.catalog.launchTargets.size());
  ImGui::Separator();
  ImGui::InputTextWithHint("##manualScene", "Custom glTF / GLB path", &launcherState.scenePath);
  if (ImGui::IsItemEdited()) {
    launcherState.selectedLaunchTarget = -1;
    updateLauncherPreview();
  }
  ImGui::Spacing();
  ImGui::BeginChild("LaunchTargetList", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
  for (int i = 0; i < static_cast<int>(launcherState.catalog.launchTargets.size()); ++i) {
    const auto& entry = launcherState.catalog.launchTargets[static_cast<size_t>(i)];
    const bool selected = launcherState.selectedLaunchTarget == i;
    if (ImGui::Selectable((entry.label + "##launch-target").c_str(), selected)) {
      launcherState.selectedLaunchTarget = i;
      launcherState.scenePath = entry.path;
      updateLauncherPreview();
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.47f, 0.58f, 0.68f, 1.0f), "[%s]", entry.group.c_str());
    ImGui::TextDisabled("%s", entry.description.c_str());
  }
  ImGui::EndChild();
  endCard();

  ImGui::SameLine(0.0f, 20.0f);

  beginCard("LaunchConfigCard", ImVec2(rightWidth, cardHeight));
  ImGui::TextUnformatted("Environment and flags");
  ImGui::TextColored(ImVec4(0.46f, 0.84f, 0.74f, 1.0f), "%zu lighting presets discovered", launcherState.catalog.iblMaps.size());
  ImGui::Separator();
  ImGui::InputTextWithHint("##manualIbl", "Custom HDR / environment map path", &launcherState.iblPath);
  if (ImGui::IsItemEdited()) {
    launcherState.selectedIblMap = -1;
    updateLauncherPreview();
  }
  ImGui::Spacing();
  ImGui::BeginChild("IblList", ImVec2(0.0f, 168.0f), ImGuiChildFlags_Borders);
  for (int i = 0; i < static_cast<int>(launcherState.catalog.iblMaps.size()); ++i) {
    const auto& entry = launcherState.catalog.iblMaps[static_cast<size_t>(i)];
    const bool selected = launcherState.selectedIblMap == i;
    if (ImGui::Selectable((entry.label + "##ibl-target").c_str(), selected)) {
      launcherState.selectedIblMap = i;
      launcherState.iblPath = entry.path;
      updateLauncherPreview();
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.47f, 0.58f, 0.68f, 1.0f), "[%s]", entry.group.c_str());
    ImGui::TextDisabled("%s", entry.description.c_str());
  }
  ImGui::EndChild();

  ImGui::Spacing();
  const char* resolutionLabel =
      launcherState.selectedResolutionPreset >= 0
          ? kLauncherResolutionOptions[static_cast<size_t>(launcherState.selectedResolutionPreset)].label
          : "Custom";
  if (ImGui::BeginCombo("Resolution", resolutionLabel)) {
    for (int i = 0; i < static_cast<int>(kLauncherResolutionOptions.size()); ++i) {
      const bool selected = launcherState.selectedResolutionPreset == i;
      const auto& preset = kLauncherResolutionOptions[static_cast<size_t>(i)];
      if (ImGui::Selectable(preset.label, selected)) {
        launcherState.selectedResolutionPreset = i;
        launcherState.launchWidth = preset.width;
        launcherState.launchHeight = preset.height;
        updateLauncherPreview();
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    const bool customSelected = launcherState.selectedResolutionPreset < 0;
    if (ImGui::Selectable("Custom", customSelected)) {
      launcherState.selectedResolutionPreset = -1;
    }
    ImGui::EndCombo();
  }

  int resolutionValues[2] = {
      static_cast<int>(launcherState.launchWidth),
      static_cast<int>(launcherState.launchHeight),
  };
  if (ImGui::InputInt2("Window size", resolutionValues)) {
    launcherState.launchWidth = static_cast<uint32_t>(std::clamp(resolutionValues[0], 640, 7680));
    launcherState.launchHeight = static_cast<uint32_t>(std::clamp(resolutionValues[1], 360, 4320));
    launcherState.selectedResolutionPreset =
        findResolutionPreset(launcherState.launchWidth, launcherState.launchHeight);
    updateLauncherPreview();
  }

  ImGui::TextDisabled("Resolution is applied when you leave the launcher.");
  ImGui::Spacing();
  ImGui::TextUnformatted("Command line preview");
  ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(11, 14, 20, 220));
  ImGui::InputTextMultiline("##command-preview", &launcherState.commandPreview, ImVec2(-1.0f, 116.0f), ImGuiInputTextFlags_ReadOnly);
  ImGui::PopStyleColor();
  if (!launcherState.statusMessage.empty()) {
    ImGui::Spacing();
    ImGui::TextWrapped("%s", launcherState.statusMessage.c_str());
  }
  endCard();

  ImGui::SetCursorPosY(viewport->WorkSize.y - 74.0f);
  if (ImGui::Button("Launch Scene", ImVec2(180.0f, 42.0f))) {
    launchFromLauncher();
  }
  ImGui::SameLine();
  if (ImGui::Button("Quit", ImVec2(120.0f, 42.0f))) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
  ImGui::SameLine();
  ImGui::TextColored(ImVec4(0.47f, 0.58f, 0.68f, 1.0f), "Pick a scene and hand off into the runtime.");

  ImGui::PopStyleColor();
  ImGui::End();
  ImGui::PopStyleVar(2);
}

void SauceEngineApp::updateLauncherPreview() {
  AppOptions options;
  options.scr_width = launcherState.launchWidth;
  options.scr_height = launcherState.launchHeight;
  options.scene_file = launcherState.scenePath;
  options.ibl_file = launcherState.iblPath;
  options.skip_launcher = true;
  launcherState.commandPreview = sauce::launcher::buildCommandLinePreview(options);
}

bool SauceEngineApp::launchFromLauncher() {
  if (!launcherState.scenePath.empty() && !fs::exists(launcherState.scenePath)) {
    launcherState.statusMessage = "Scene/model file not found: " + launcherState.scenePath;
    return false;
  }

  if (!launcherState.iblPath.empty() && !fs::exists(launcherState.iblPath)) {
    launcherState.statusMessage = "Environment map not found: " + launcherState.iblPath;
    return false;
  }

  const bool resolutionChanged =
      launcherState.launchWidth != width || launcherState.launchHeight != height;

  width = launcherState.launchWidth;
  height = launcherState.launchHeight;
  sceneFile = launcherState.scenePath.empty() ? kDefaultLauncherScene : launcherState.scenePath;
  iblFile = launcherState.iblPath;

  if (!loadConfiguredScene()) {
    launcherState.statusMessage = "Failed to load the selected scene. Check the console for loader errors.";
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
  launcherState.statusMessage = "Launching scene.";
  glfwSetWindowTitle(window, kEngineWindowTitle);
  setCursorCapture(true);
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

  bool firstDraw = true;

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
        .loadOp = firstDraw ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor,
    };
    vk::RenderingAttachmentInfo depthAttachment{
        .imageView = pRenderer->getDepthImageView(),
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = firstDraw ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
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
    firstDraw = false;
  }

  if (firstDraw) {
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
        .storeOp = vk::AttachmentStoreOp::eDontCare,
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
