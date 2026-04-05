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
#include <cstring>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <sstream>

namespace sauce {

namespace {

namespace fs = std::filesystem;

constexpr const char* kLauncherWindowTitle = "SauceEngine Launcher";
constexpr const char* kEngineWindowTitle = "SauceEngine";
constexpr const char* kDefaultLauncherScene = "assets/models/Cube.gltf";
constexpr const char* kRemoteSceneTabLabel = "Poly Haven";
constexpr const char* kLocalSceneTabLabel = "Local Library";
constexpr const char* kPolyHavenLoadingMessage = "Contacting Poly Haven...";

struct LauncherResolutionOption {
  const char* label;
  uint32_t width;
  uint32_t height;
};

struct LauncherStringOption {
  const char* label;
  const char* value;
};

constexpr std::array<LauncherResolutionOption, 6> kLauncherResolutionOptions{{
    {"1280 x 720", 1280, 720},
    {"1600 x 900", 1600, 900},
    {"1920 x 1080", 1920, 1080},
    {"2560 x 1440", 2560, 1440},
    {"3024 x 1964", 3024, 1964},
    {"3840 x 2160", 3840, 2160},
}};

constexpr std::array<LauncherStringOption, 3> kModelResolutionOptions{{
    {"1k textures", "1k"},
    {"2k textures", "2k"},
    {"4k textures", "4k"},
}};

constexpr std::array<LauncherStringOption, 5> kHdriResolutionOptions{{
    {"1k", "1k"},
    {"2k", "2k"},
    {"4k", "4k"},
    {"8k", "8k"},
    {"16k", "16k"},
}};

constexpr std::array<const char*, 3> kRemoteSortOptions{{
    "Name",
    "Most Popular",
    "Highest Resolution",
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

int launcherResolutionRank(const std::string& resolution) {
  if (resolution == "1k") {
    return 1;
  }
  if (resolution == "2k") {
    return 2;
  }
  if (resolution == "4k") {
    return 4;
  }
  if (resolution == "8k") {
    return 8;
  }
  if (resolution == "16k") {
    return 16;
  }
  return 0;
}

int findStringOptionIndex(const auto& options, const std::string& value, int fallbackIndex) {
  for (size_t i = 0; i < options.size(); ++i) {
    if (options[i].value == value) {
      return static_cast<int>(i);
    }
  }
  return fallbackIndex;
}

std::string lowercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::string normalizeSearchText(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    if (std::isalnum(ch)) {
      return static_cast<char>(std::tolower(ch));
    }
    return ' ';
  });
  return value;
}

std::string joinStrings(const std::vector<std::string>& values, const std::string& separator) {
  std::ostringstream stream;
  for (size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      stream << separator;
    }
    stream << values[i];
  }
  return stream.str();
}

bool matchesRemoteSearch(const sauce::launcher::PolyHavenAssetSummary& asset, const std::string& query) {
  if (query.empty()) {
    return true;
  }

  std::ostringstream haystackStream;
  haystackStream << asset.id << ' ' << asset.name << ' ' << asset.description << ' ';
  for (const std::string& category : asset.categories) {
    haystackStream << category << ' ';
  }
  for (const std::string& tag : asset.tags) {
    haystackStream << tag << ' ';
  }

  const std::string haystack = normalizeSearchText(haystackStream.str());
  std::istringstream terms(normalizeSearchText(query));
  std::string term;
  while (terms >> term) {
    if (haystack.find(term) == std::string::npos) {
      return false;
    }
  }
  return true;
}

int remoteResolutionRank(const sauce::launcher::PolyHavenAssetSummary& asset) {
  return launcherResolutionRank(asset.maxResolutionLabel);
}

std::string formatBytes(uintmax_t bytes) {
  static constexpr std::array<const char*, 5> kUnits{"B", "KB", "MB", "GB", "TB"};
  double value = static_cast<double>(bytes);
  size_t unitIndex = 0;
  while (value >= 1024.0 && unitIndex + 1 < kUnits.size()) {
    value /= 1024.0;
    ++unitIndex;
  }

  std::ostringstream stream;
  stream << std::fixed << std::setprecision(unitIndex == 0 ? 0 : 1) << value << ' ' << kUnits[unitIndex];
  return stream.str();
}

bool sortedContains(const std::vector<std::string>& values, const std::string& value) {
  return std::binary_search(values.begin(), values.end(), value);
}

const sauce::launcher::PolyHavenAssetSummary* findAssetById(
    const std::vector<sauce::launcher::PolyHavenAssetSummary>& assets,
    const std::string& id) {
  if (id.empty()) {
    return nullptr;
  }

  const auto it = std::find_if(assets.begin(), assets.end(), [&](const sauce::launcher::PolyHavenAssetSummary& asset) {
    return asset.id == id;
  });
  return it == assets.end() ? nullptr : &(*it);
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
    launcherState.selectedSceneTab = polyHavenModelId.empty() ? 0 : 1;
    launcherState.selectedIblTab = polyHavenHdriId.empty() ? 0 : 1;
    launcherState.scenePath = launcherState.catalog.launchTargets.empty()
        ? std::string()
        : launcherState.catalog.launchTargets.front().path;
    launcherState.iblPath = launcherState.catalog.iblMaps.empty()
        ? std::string()
        : launcherState.catalog.iblMaps.front().path;
    launcherState.remoteModels.selectedResolutionIndex =
        findStringOptionIndex(kModelResolutionOptions, polyHavenModelResolution, 1);
    launcherState.remoteHdris.selectedResolutionIndex =
        findStringOptionIndex(kHdriResolutionOptions, polyHavenHdriResolution, 2);
    launcherState.statusMessage = "Pick a scene, choose lighting, then launch into the runtime.";
    refreshLauncherCacheState();
    updateLauncherPreview();
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
    }

    pImGuiRenderer->newFrame();
    pollLauncherBackgroundTasks();
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

void SauceEngineApp::pollLauncherBackgroundTasks() {
  auto pollCatalog = [&](RemoteBrowserState& browserState) {
    if (!browserState.pendingLoad.has_value()) {
      return;
    }

    auto& future = *browserState.pendingLoad;
    if (future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
      return;
    }

    auto result = future.get();
    browserState.pendingLoad.reset();
    if (!result.errorMessage.empty()) {
      browserState.statusMessage = result.errorMessage;
      return;
    }

    browserState.assets = std::move(result.assets);
    browserState.statusMessage = "Loaded " + std::to_string(browserState.assets.size()) + " assets from Poly Haven.";
  };

  pollCatalog(launcherState.remoteModels);
  pollCatalog(launcherState.remoteHdris);

  if (!launcherState.pendingLaunch.has_value()) {
    return;
  }

  auto& future = *launcherState.pendingLaunch;
  if (future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
    return;
  }

  PendingLaunchResult result = future.get();
  launcherState.pendingLaunch.reset();
  if (!result.errorMessage.empty()) {
    launcherState.statusMessage = result.errorMessage;
    return;
  }

  refreshLauncherCacheState();
  updateLauncherPreview();
  finalizeLauncherLaunch(result.scenePath, result.iblPath);
}

void SauceEngineApp::refreshLauncherCacheState() {
  launcherState.cache.summary = sauce::launcher::inspectPolyHavenCache();
  if (launcherState.cache.summary.totalBytes == 0) {
    launcherState.cache.statusMessage = "Cache is empty.";
  } else {
    launcherState.cache.statusMessage =
        formatBytes(launcherState.cache.summary.totalBytes) + " across " +
        std::to_string(launcherState.cache.summary.fileCount) + " files.";
  }
}

void SauceEngineApp::renderLauncherUI() {
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImDrawList* drawList = ImGui::GetBackgroundDrawList(viewport);
  const ImVec2 min = viewport->WorkPos;
  const ImVec2 max = ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y);

  struct LauncherTheme {
    ImU32 bgTop = IM_COL32(18, 24, 35, 255);
    ImU32 bgBottom = IM_COL32(9, 11, 17, 255);
    ImU32 bgGrid = IM_COL32(120, 150, 190, 14);
    ImU32 frame = IM_COL32(20, 28, 40, 236);
    ImU32 frameAlt = IM_COL32(16, 22, 33, 244);
    ImU32 frameStrong = IM_COL32(26, 36, 52, 250);
    ImU32 border = IM_COL32(78, 94, 120, 210);
    ImU32 accent = IM_COL32(129, 181, 255, 255);
    ImU32 accentSoft = IM_COL32(110, 146, 196, 255);
    ImU32 success = IM_COL32(119, 202, 172, 255);
    ImVec4 text = ImVec4(0.93f, 0.95f, 0.98f, 1.0f);
    ImVec4 muted = ImVec4(0.58f, 0.66f, 0.76f, 1.0f);
    ImVec4 subtle = ImVec4(0.47f, 0.55f, 0.64f, 1.0f);
    float gap = 18.0f;
  } theme;

  drawList->AddRectFilledMultiColor(min, max, theme.bgTop, theme.bgTop, theme.bgBottom, theme.bgBottom);
  for (float x = min.x - 32.0f; x < max.x + 32.0f; x += 48.0f) {
    drawList->AddLine(ImVec2(x, min.y), ImVec2(x + 120.0f, max.y), theme.bgGrid, 1.0f);
  }
  drawList->AddRectFilled(
      ImVec2(max.x - 260.0f, min.y),
      ImVec2(max.x, min.y + 6.0f),
      theme.accent);
  drawList->AddRectFilled(
      ImVec2(min.x, max.y - 4.0f),
      ImVec2(min.x + 220.0f, max.y),
      IM_COL32(92, 120, 168, 180));

  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(28.0f, 22.0f));

  const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                                 ImGuiWindowFlags_NoMove |
                                 ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoSavedSettings;
  ImGui::Begin("SauceLauncher", nullptr, flags);

  constexpr int kLauncherColorPushCount = 18;
  ImGui::PushStyleColor(ImGuiCol_Text, theme.text);
  ImGui::PushStyleColor(ImGuiCol_TextDisabled, theme.muted);
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::ColorConvertU32ToFloat4(theme.frame));
  ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(theme.border));
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(theme.frameStrong));
  ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.18f, 0.25f, 0.37f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.22f, 0.31f, 0.45f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.19f, 0.28f, 0.42f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.26f, 0.38f, 0.56f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.34f, 0.55f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.17f, 0.24f, 0.36f, 0.88f));
  ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.23f, 0.34f, 0.49f, 0.95f));
  ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.29f, 0.42f, 0.60f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.12f, 0.17f, 0.25f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.20f, 0.29f, 0.42f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_TabSelected, ImVec4(0.25f, 0.37f, 0.54f, 1.0f));
  ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.30f, 0.37f, 0.48f, 0.85f));
  ImGui::PushStyleColor(ImGuiCol_CheckMark, ImGui::ColorConvertU32ToFloat4(theme.accent));

  auto beginSurface = [&](const char* id, const ImVec2& size, const ImVec2& padding = ImVec2(18.0f, 16.0f)) {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
    ImGui::BeginChild(id, size, ImGuiChildFlags_Borders);
  };
  auto endSurface = [&]() {
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
  };
  auto renderSectionEyebrow = [&](const char* label, const ImVec4& color) {
    ImGui::TextColored(color, "%s", label);
    ImGui::Separator();
  };
  auto selectedSceneAsset = findAssetById(launcherState.remoteModels.assets, polyHavenModelId);
  auto selectedHdriAsset = findAssetById(launcherState.remoteHdris.assets, polyHavenHdriId);
  const bool selectedModelCached = !polyHavenModelId.empty() &&
                                   sortedContains(launcherState.cache.summary.cachedModelIds, polyHavenModelId);
  const bool selectedHdriCached = !polyHavenHdriId.empty() &&
                                  sortedContains(launcherState.cache.summary.cachedHdriIds, polyHavenHdriId);

  auto focusedRemoteAsset = [&]() -> const sauce::launcher::PolyHavenAssetSummary* {
    if (launcherState.selectedSceneTab == 1 && selectedSceneAsset != nullptr) {
      return selectedSceneAsset;
    }
    if (launcherState.selectedIblTab == 1 && selectedHdriAsset != nullptr) {
      return selectedHdriAsset;
    }
    return selectedSceneAsset != nullptr ? selectedSceneAsset : selectedHdriAsset;
  }();

  auto focusedRemoteAssetLabel = [&]() -> const char* {
    if (launcherState.selectedSceneTab == 1 && selectedSceneAsset != nullptr) {
      return "Focused remote model";
    }
    if (launcherState.selectedIblTab == 1 && selectedHdriAsset != nullptr) {
      return "Focused remote HDRI";
    }
    if (selectedSceneAsset != nullptr) {
      return "Focused remote model";
    }
    if (selectedHdriAsset != nullptr) {
      return "Focused remote HDRI";
    }
    return "Focused remote asset";
  }();

  auto describeLocalSelection = [](const std::string& value, const char* fallback) {
    return value.empty() ? std::string(fallback) : value;
  };
  auto applyCacheMutation = [&](const sauce::launcher::PolyHavenCacheMutationResult& result, const char* emptyMessage, const char* successPrefix) {
    if (!result.errorMessage.empty()) {
      launcherState.cache.statusMessage = result.errorMessage;
      return;
    }

    refreshLauncherCacheState();
    if (result.assetsRemoved == 0 && result.bytesRemoved == 0) {
      launcherState.cache.statusMessage = emptyMessage;
    } else {
      launcherState.cache.statusMessage =
          std::string(successPrefix) + " Freed " + formatBytes(result.bytesRemoved) + ".";
    }
    updateLauncherPreview();
  };

  ImGui::TextColored(theme.muted, "SAUCE ENGINE");
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
  ImGui::SetWindowFontScale(1.35f);
  ImGui::TextUnformatted("Launch Setup");
  ImGui::SetWindowFontScale(1.0f);
  ImGui::TextDisabled("Fast scene boot for developers. Local files and Poly Haven assets in one flow.");
  ImGui::Spacing();
  ImGui::TextColored(
      ImVec4(0.74f, 0.84f, 0.95f, 1.0f),
      "%zu local targets  |  %zu local maps  |  %u x %u  |  %s cache",
      launcherState.catalog.launchTargets.size(),
      launcherState.catalog.iblMaps.size(),
      launcherState.launchWidth,
      launcherState.launchHeight,
      formatBytes(launcherState.cache.summary.totalBytes).c_str());
  ImGui::Dummy(ImVec2(0.0f, 8.0f));

  const float fullWidth = ImGui::GetContentRegionAvail().x;
  const float footerHeight = 78.0f;
  const float contentHeight = std::max(360.0f, ImGui::GetContentRegionAvail().y - footerHeight - theme.gap);
  const bool stackedLayout = fullWidth < 1180.0f;
  const float sidebarWidth = stackedLayout
      ? fullWidth
      : std::clamp(fullWidth * 0.34f, 360.0f, 460.0f);
  const float browserWidth = stackedLayout
      ? fullWidth
      : std::max(420.0f, fullWidth - sidebarWidth - theme.gap);
  const float browserHeight = stackedLayout
      ? std::max(240.0f, (contentHeight - (theme.gap * 2.0f)) / 3.0f)
      : std::max(240.0f, (contentHeight - theme.gap) * 0.5f);
  const float sidebarHeight = stackedLayout
      ? std::max(260.0f, contentHeight - (browserHeight * 2.0f) - (theme.gap * 2.0f))
      : contentHeight;
  const std::string cacheRootText = sauce::launcher::getPolyHavenCacheRoot().string();

  auto startRemoteLoadIfNeeded = [&](RemoteBrowserState& browserState, sauce::launcher::PolyHavenAssetType type) {
    if (!browserState.assets.empty() || browserState.pendingLoad.has_value() || !browserState.statusMessage.empty()) {
      return;
    }

    browserState.statusMessage = kPolyHavenLoadingMessage;
    browserState.pendingLoad = std::async(std::launch::async, [type]() {
      return sauce::launcher::fetchPolyHavenAssets(type);
    });
  };

  auto renderRemoteAssetList = [&](RemoteBrowserState& browserState,
                                   const auto& resolutionOptions,
                                   int& resolutionIndex,
                                   std::string& selectedId,
                                   std::string& selectedResolution,
                                   std::string& localPathToClear,
                                   int& selectedLocalIndex,
                                   const char* searchLabel,
                                   const char* refreshLabel,
                                   const char* presetLabel,
                                   sauce::launcher::PolyHavenAssetType assetType,
                                   const char* listId,
                                   const std::vector<std::string>& cachedIds) {
    startRemoteLoadIfNeeded(browserState, assetType);

    if (ImGui::Button(refreshLabel)) {
      browserState.assets.clear();
      browserState.selectedIndex = -1;
      browserState.statusMessage = kPolyHavenLoadingMessage;
      browserState.pendingLoad = std::async(std::launch::async, [assetType]() {
        return sauce::launcher::fetchPolyHavenAssets(assetType);
      });
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(std::min(260.0f, ImGui::GetContentRegionAvail().x * 0.42f));
    ImGui::InputTextWithHint(searchLabel, "Filter by name, tag, or category", &browserState.searchQuery);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(150.0f);
    const std::string sortLabel = std::string("Sort##") + listId;
    ImGui::Combo(sortLabel.c_str(), &browserState.selectedSortMode, kRemoteSortOptions.data(), static_cast<int>(kRemoteSortOptions.size()));

    const int safeResolutionIndex = std::clamp(resolutionIndex, 0, static_cast<int>(resolutionOptions.size()) - 1);
    const char* resolutionLabel = resolutionOptions[static_cast<size_t>(safeResolutionIndex)].label;
    ImGui::SetNextItemWidth(180.0f);
    if (ImGui::BeginCombo(presetLabel, resolutionLabel)) {
      for (int i = 0; i < static_cast<int>(resolutionOptions.size()); ++i) {
        const bool selected = resolutionIndex == i;
        if (ImGui::Selectable(resolutionOptions[static_cast<size_t>(i)].label, selected)) {
          resolutionIndex = i;
          selectedResolution = resolutionOptions[static_cast<size_t>(i)].value;
          updateLauncherPreview();
        }
        if (selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::TextColored(theme.subtle, "%s", browserState.statusMessage.c_str());
    ImGui::Dummy(ImVec2(0.0f, 6.0f));

    std::vector<int> visibleIndices;
    visibleIndices.reserve(browserState.assets.size());
    for (int i = 0; i < static_cast<int>(browserState.assets.size()); ++i) {
      if (matchesRemoteSearch(browserState.assets[static_cast<size_t>(i)], browserState.searchQuery)) {
        visibleIndices.push_back(i);
      }
    }

    std::sort(visibleIndices.begin(), visibleIndices.end(), [&](int lhsIndex, int rhsIndex) {
      const auto& lhs = browserState.assets[static_cast<size_t>(lhsIndex)];
      const auto& rhs = browserState.assets[static_cast<size_t>(rhsIndex)];
      switch (browserState.selectedSortMode) {
        case 1:
          if (lhs.downloadCount != rhs.downloadCount) {
            return lhs.downloadCount > rhs.downloadCount;
          }
          return lhs.name < rhs.name;
        case 2:
          if (remoteResolutionRank(lhs) != remoteResolutionRank(rhs)) {
            return remoteResolutionRank(lhs) > remoteResolutionRank(rhs);
          }
          if (lhs.downloadCount != rhs.downloadCount) {
            return lhs.downloadCount > rhs.downloadCount;
          }
          return lhs.name < rhs.name;
        case 0:
        default:
          return lhs.name < rhs.name;
      }
    });

    ImGui::BeginChild(listId, ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
    if (browserState.pendingLoad.has_value()) {
      ImGui::TextDisabled("Loading asset list...");
    }

    for (const int i : visibleIndices) {
      const auto& asset = browserState.assets[static_cast<size_t>(i)];
      const bool selected = selectedId == asset.id;
      const bool cached = sortedContains(cachedIds, asset.id);

      std::ostringstream rowLabel;
      rowLabel << asset.name;
      if (!asset.maxResolutionLabel.empty()) {
        rowLabel << "  |  " << asset.maxResolutionLabel;
      }
      rowLabel << "  |  " << asset.downloadCount << " dl";
      if (cached) {
        rowLabel << "  |  cached";
      }

      if (ImGui::Selectable((rowLabel.str() + "##" + asset.id).c_str(), selected)) {
        browserState.selectedIndex = i;
        selectedId = asset.id;
        selectedResolution = resolutionOptions[static_cast<size_t>(safeResolutionIndex)].value;
        localPathToClear.clear();
        selectedLocalIndex = -1;
        updateLauncherPreview();
      }
    }

    if (visibleIndices.empty() && !browserState.pendingLoad.has_value()) {
      ImGui::TextDisabled("No assets match the current filter.");
    }
    ImGui::EndChild();
  };

  if (!stackedLayout) {
    ImGui::BeginGroup();
  }

  beginSurface("LaunchTargetsCard", ImVec2(browserWidth, browserHeight));
  renderSectionEyebrow("SCENE / MODEL", ImGui::ColorConvertU32ToFloat4(theme.accent));
  ImGui::TextDisabled("Choose a local scene or browse remote Poly Haven models.");
  if (ImGui::BeginTabBar("SceneSourceTabs")) {
    if (ImGui::BeginTabItem(kLocalSceneTabLabel)) {
      launcherState.selectedSceneTab = 0;
      ImGui::InputTextWithHint("##manualScene", "Custom glTF / GLB path", &launcherState.scenePath);
      if (ImGui::IsItemEdited()) {
        launcherState.selectedLaunchTarget = -1;
        polyHavenModelId.clear();
        updateLauncherPreview();
      }
      ImGui::Dummy(ImVec2(0.0f, 6.0f));
      ImGui::BeginChild("LaunchTargetList", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
      for (int i = 0; i < static_cast<int>(launcherState.catalog.launchTargets.size()); ++i) {
        const auto& entry = launcherState.catalog.launchTargets[static_cast<size_t>(i)];
        const bool selected = launcherState.selectedLaunchTarget == i && polyHavenModelId.empty();
        if (ImGui::Selectable((entry.label + "##launch-target").c_str(), selected)) {
          launcherState.selectedLaunchTarget = i;
          launcherState.scenePath = entry.path;
          polyHavenModelId.clear();
          updateLauncherPreview();
        }
        ImGui::SameLine();
        ImGui::TextColored(theme.subtle, "[%s]  %s", entry.group.c_str(), entry.description.c_str());
      }
      ImGui::EndChild();
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(kRemoteSceneTabLabel)) {
      launcherState.selectedSceneTab = 1;
      renderRemoteAssetList(
          launcherState.remoteModels,
          kModelResolutionOptions,
          launcherState.remoteModels.selectedResolutionIndex,
          polyHavenModelId,
          polyHavenModelResolution,
          launcherState.scenePath,
          launcherState.selectedLaunchTarget,
          "##polyhaven-model-search",
          "Refresh Models",
          "Download preset##polyhaven-model-preset",
          sauce::launcher::PolyHavenAssetType::Model,
          "PolyHavenModelList",
          launcherState.cache.summary.cachedModelIds);
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
  endSurface();

  ImGui::Dummy(ImVec2(0.0f, theme.gap));

  beginSurface("LaunchConfigCard", ImVec2(browserWidth, browserHeight));
  renderSectionEyebrow("ENVIRONMENT / IBL", ImGui::ColorConvertU32ToFloat4(theme.success));
  ImGui::TextDisabled("Pair the scene with a local HDR or a remote Poly Haven environment.");
  if (ImGui::BeginTabBar("IblSourceTabs")) {
    if (ImGui::BeginTabItem(kLocalSceneTabLabel)) {
      launcherState.selectedIblTab = 0;
      ImGui::InputTextWithHint("##manualIbl", "Custom HDR / environment map path", &launcherState.iblPath);
      if (ImGui::IsItemEdited()) {
        launcherState.selectedIblMap = -1;
        polyHavenHdriId.clear();
        updateLauncherPreview();
      }
      ImGui::Dummy(ImVec2(0.0f, 6.0f));
      ImGui::BeginChild("IblList", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
      for (int i = 0; i < static_cast<int>(launcherState.catalog.iblMaps.size()); ++i) {
        const auto& entry = launcherState.catalog.iblMaps[static_cast<size_t>(i)];
        const bool selected = launcherState.selectedIblMap == i && polyHavenHdriId.empty();
        if (ImGui::Selectable((entry.label + "##ibl-target").c_str(), selected)) {
          launcherState.selectedIblMap = i;
          launcherState.iblPath = entry.path;
          polyHavenHdriId.clear();
          updateLauncherPreview();
        }
        ImGui::SameLine();
        ImGui::TextColored(theme.subtle, "[%s]  %s", entry.group.c_str(), entry.description.c_str());
      }
      ImGui::EndChild();
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(kRemoteSceneTabLabel)) {
      launcherState.selectedIblTab = 1;
      renderRemoteAssetList(
          launcherState.remoteHdris,
          kHdriResolutionOptions,
          launcherState.remoteHdris.selectedResolutionIndex,
          polyHavenHdriId,
          polyHavenHdriResolution,
          launcherState.iblPath,
          launcherState.selectedIblMap,
          "##polyhaven-hdri-search",
          "Refresh HDRIs",
          "Download preset##polyhaven-hdri-preset",
          sauce::launcher::PolyHavenAssetType::Hdri,
          "PolyHavenHdriList",
          launcherState.cache.summary.cachedHdriIds);
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
  endSurface();

  if (!stackedLayout) {
    ImGui::EndGroup();
    ImGui::SameLine(0.0f, theme.gap);
  } else {
    ImGui::Dummy(ImVec2(0.0f, theme.gap));
  }

  beginSurface("LaunchSummaryPanel", ImVec2(stackedLayout ? fullWidth : sidebarWidth, sidebarHeight));
  renderSectionEyebrow("SUMMARY / RUNTIME", ImGui::ColorConvertU32ToFloat4(theme.accentSoft));

  ImGui::TextUnformatted("Selection");
  ImGui::TextColored(theme.subtle, "Scene");
  if (polyHavenModelId.empty()) {
    ImGui::TextWrapped("%s", describeLocalSelection(launcherState.scenePath, kDefaultLauncherScene).c_str());
  } else if (selectedSceneAsset != nullptr) {
    ImGui::TextWrapped("%s", selectedSceneAsset->name.c_str());
    ImGui::TextDisabled(
        "Poly Haven  |  %s  |  %s%s",
        polyHavenModelId.c_str(),
        polyHavenModelResolution.c_str(),
        selectedModelCached ? "  |  cached" : "");
  } else {
    ImGui::TextWrapped("%s", polyHavenModelId.c_str());
  }

  ImGui::Dummy(ImVec2(0.0f, 8.0f));
  ImGui::TextColored(theme.subtle, "Environment");
  if (polyHavenHdriId.empty()) {
    ImGui::TextWrapped("%s", launcherState.iblPath.empty() ? "Engine default lighting" : launcherState.iblPath.c_str());
  } else if (selectedHdriAsset != nullptr) {
    ImGui::TextWrapped("%s", selectedHdriAsset->name.c_str());
    ImGui::TextDisabled(
        "Poly Haven  |  %s  |  %s%s",
        polyHavenHdriId.c_str(),
        polyHavenHdriResolution.c_str(),
        selectedHdriCached ? "  |  cached" : "");
  } else {
    ImGui::TextWrapped("%s", polyHavenHdriId.c_str());
  }

  ImGui::Dummy(ImVec2(0.0f, 12.0f));
  ImGui::Separator();
  ImGui::Dummy(ImVec2(0.0f, 10.0f));

  ImGui::TextUnformatted(focusedRemoteAssetLabel);
  if (focusedRemoteAsset != nullptr) {
    ImGui::TextDisabled(
        "%s  |  %s  |  %d downloads",
        focusedRemoteAsset->id.c_str(),
        focusedRemoteAsset->maxResolutionLabel.empty() ? "max res ?" : focusedRemoteAsset->maxResolutionLabel.c_str(),
        focusedRemoteAsset->downloadCount);
    if (!focusedRemoteAsset->categories.empty()) {
      ImGui::TextDisabled("Categories: %s", joinStrings(focusedRemoteAsset->categories, ", ").c_str());
    }
    if (!focusedRemoteAsset->tags.empty()) {
      ImGui::TextDisabled("Tags: %s", joinStrings(focusedRemoteAsset->tags, ", ").c_str());
    }
    if (!focusedRemoteAsset->description.empty()) {
      ImGui::Dummy(ImVec2(0.0f, 4.0f));
      ImGui::TextWrapped("%s", focusedRemoteAsset->description.c_str());
    }
  } else {
    ImGui::TextDisabled("Select a remote model or HDRI to inspect its details here.");
  }

  ImGui::Dummy(ImVec2(0.0f, 12.0f));
  ImGui::Separator();
  ImGui::Dummy(ImVec2(0.0f, 10.0f));

  ImGui::TextUnformatted("Runtime");
  const char* resolutionLabel =
      launcherState.selectedResolutionPreset >= 0
          ? kLauncherResolutionOptions[static_cast<size_t>(launcherState.selectedResolutionPreset)].label
          : "Custom";
  if (ImGui::BeginCombo("Launch resolution", resolutionLabel)) {
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

  ImGui::Dummy(ImVec2(0.0f, 12.0f));
  ImGui::Separator();
  ImGui::Dummy(ImVec2(0.0f, 10.0f));

  ImGui::TextUnformatted("Cache");
  ImGui::TextColored(theme.subtle, "%s", launcherState.cache.statusMessage.c_str());
  ImGui::TextDisabled(
      "%zu models  |  %zu HDRIs  |  %zu files",
      launcherState.cache.summary.modelAssetCount,
      launcherState.cache.summary.hdriAssetCount,
      launcherState.cache.summary.fileCount);

  launcherState.cache.pruneDays = std::clamp(launcherState.cache.pruneDays, 1, 365);
  if (ImGui::InputInt("Prune after days", &launcherState.cache.pruneDays)) {
    launcherState.cache.pruneDays = std::clamp(launcherState.cache.pruneDays, 1, 365);
  }

  if (ImGui::Button("Refresh cache")) {
    refreshLauncherCacheState();
  }
  ImGui::SameLine();
  if (ImGui::Button("Prune stale")) {
    applyCacheMutation(
        sauce::launcher::prunePolyHavenCacheOlderThan(std::chrono::hours(24 * launcherState.cache.pruneDays)),
        "No stale cached assets found.",
        "Pruned stale assets.");
  }
  if (ImGui::Button("Clear cache")) {
    applyCacheMutation(
        sauce::launcher::clearPolyHavenCache(),
        "Cache was already empty.",
        "Cleared Poly Haven cache.");
  }

  if (selectedModelCached) {
    ImGui::SameLine();
    if (ImGui::Button("Remove model")) {
      applyCacheMutation(
          sauce::launcher::removePolyHavenCachedAsset(sauce::launcher::PolyHavenAssetType::Model, polyHavenModelId),
          "Selected model was not cached.",
          "Removed cached model.");
    }
  }
  if (selectedHdriCached) {
    ImGui::SameLine();
    if (ImGui::Button("Remove HDRI")) {
      applyCacheMutation(
          sauce::launcher::removePolyHavenCachedAsset(sauce::launcher::PolyHavenAssetType::Hdri, polyHavenHdriId),
          "Selected HDRI was not cached.",
          "Removed cached HDRI.");
    }
  }

  ImGui::TextDisabled("%s", cacheRootText.c_str());

  ImGui::Dummy(ImVec2(0.0f, 12.0f));
  ImGui::Separator();
  ImGui::Dummy(ImVec2(0.0f, 10.0f));

  ImGui::TextUnformatted("Command preview");
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(theme.frameAlt));
  ImGui::InputTextMultiline("##command-preview", &launcherState.commandPreview, ImVec2(-1.0f, 116.0f), ImGuiInputTextFlags_ReadOnly);
  ImGui::PopStyleColor();
  endSurface();

  ImGui::Dummy(ImVec2(0.0f, theme.gap));
  const bool launchBusy = launcherState.pendingLaunch.has_value();
  beginSurface("LauncherFooter", ImVec2(0.0f, footerHeight), ImVec2(18.0f, 14.0f));
  ImGui::BeginGroup();
  ImGui::TextUnformatted(launchBusy ? "Preparing runtime handoff" : "Ready to launch");
  ImGui::TextColored(
      theme.subtle,
      "%s",
      launchBusy ? "Downloading selected remote assets into the local cache." : launcherState.statusMessage.c_str());
  ImGui::EndGroup();

  const float footerButtonWidth = 180.0f + 120.0f + 12.0f;
  const float footerRight = ImGui::GetWindowContentRegionMax().x;
  ImGui::SameLine();
  ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), footerRight - footerButtonWidth));
  if (launchBusy) {
    ImGui::BeginDisabled();
  }
  if (ImGui::Button(launchBusy ? "Preparing..." : "Launch Scene", ImVec2(180.0f, 42.0f))) {
    launchFromLauncher();
  }
  if (launchBusy) {
    ImGui::EndDisabled();
  }
  ImGui::SameLine();
  if (ImGui::Button("Quit", ImVec2(120.0f, 42.0f))) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }
  endSurface();

  ImGui::PopStyleColor(kLauncherColorPushCount);
  ImGui::End();
  ImGui::PopStyleVar(2);
}

void SauceEngineApp::updateLauncherPreview() {
  AppOptions options;
  options.scr_width = launcherState.launchWidth;
  options.scr_height = launcherState.launchHeight;
  if (polyHavenModelId.empty()) {
    options.scene_file = launcherState.scenePath;
  } else {
    const auto modelCachePath = sauce::launcher::getPolyHavenModelCachePath(polyHavenModelId, polyHavenModelResolution);
    if (fs::exists(modelCachePath)) {
      options.scene_file = modelCachePath.string();
    } else {
      options.polyhaven_model_id = polyHavenModelId;
      options.polyhaven_model_resolution = polyHavenModelResolution;
    }
  }

  if (polyHavenHdriId.empty()) {
    options.ibl_file = launcherState.iblPath;
  } else {
    const auto hdriCachePath = sauce::launcher::getPolyHavenHdriCachePath(polyHavenHdriId, polyHavenHdriResolution);
    if (fs::exists(hdriCachePath)) {
      options.ibl_file = hdriCachePath.string();
    } else {
      options.polyhaven_hdri_id = polyHavenHdriId;
      options.polyhaven_hdri_resolution = polyHavenHdriResolution;
    }
  }
  options.skip_launcher = true;
  launcherState.commandPreview = sauce::launcher::buildCommandLinePreview(options);
}

bool SauceEngineApp::launchFromLauncher() {
  if (launcherState.pendingLaunch.has_value()) {
    return false;
  }

  if (!polyHavenModelId.empty() || !polyHavenHdriId.empty()) {
    const std::string modelId = polyHavenModelId;
    const std::string modelResolution = polyHavenModelResolution;
    const std::string hdriId = polyHavenHdriId;
    const std::string hdriResolution = polyHavenHdriResolution;
    const std::string localScenePath = launcherState.scenePath;
    const std::string localIblPath = launcherState.iblPath;

    launcherState.statusMessage = "Downloading selected Poly Haven assets...";
    launcherState.pendingLaunch = std::async(std::launch::async, [modelId, modelResolution, hdriId, hdriResolution, localScenePath, localIblPath]() {
      PendingLaunchResult result;
      result.scenePath = localScenePath;
      result.iblPath = localIblPath;

      if (!modelId.empty()) {
        const auto modelDownload = sauce::launcher::downloadPolyHavenModelGltf(modelId, modelResolution);
        if (!modelDownload.errorMessage.empty()) {
          result.errorMessage = "Poly Haven model download failed: " + modelDownload.errorMessage;
          return result;
        }
        result.scenePath = modelDownload.localPath.string();
      }

      if (!hdriId.empty()) {
        const auto hdriDownload = sauce::launcher::downloadPolyHavenHdri(hdriId, hdriResolution);
        if (!hdriDownload.errorMessage.empty()) {
          result.errorMessage = "Poly Haven HDRI download failed: " + hdriDownload.errorMessage;
          return result;
        }
        result.iblPath = hdriDownload.localPath.string();
      }

      return result;
    });
    return false;
  }

  if (!launcherState.scenePath.empty() && !fs::exists(launcherState.scenePath)) {
    launcherState.statusMessage = "Scene/model file not found: " + launcherState.scenePath;
    return false;
  }

  if (!launcherState.iblPath.empty() && !fs::exists(launcherState.iblPath)) {
    launcherState.statusMessage = "Environment map not found: " + launcherState.iblPath;
    return false;
  }

  return finalizeLauncherLaunch(launcherState.scenePath, launcherState.iblPath);
}

bool SauceEngineApp::finalizeLauncherLaunch(const std::string& resolvedScenePath, const std::string& resolvedIblPath) {
  const bool resolutionChanged =
      launcherState.launchWidth != width || launcherState.launchHeight != height;

  width = launcherState.launchWidth;
  height = launcherState.launchHeight;
  sceneFile = resolvedScenePath.empty() ? kDefaultLauncherScene : resolvedScenePath;
  iblFile = resolvedIblPath;

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

bool SauceEngineApp::resolveConfiguredRemoteAssets(std::string& errorMessage) {
  if (!polyHavenModelId.empty()) {
    const auto modelDownload = sauce::launcher::downloadPolyHavenModelGltf(polyHavenModelId, polyHavenModelResolution);
    if (!modelDownload.errorMessage.empty()) {
      errorMessage = "Poly Haven model download failed: " + modelDownload.errorMessage;
      return false;
    }
    sceneFile = modelDownload.localPath.string();
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
