#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <cstdlib>
#include <future>
#include <optional>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <app/BufferUtils.hpp>
#include <app/GraphicsPipeline.hpp>
#include <app/Scene.hpp>
#include <app/Instance.hpp>
#include <app/PhysicalDevice.hpp>
#include <app/LogicalDevice.hpp>
#include <app/Renderer.hpp>
#include <app/RenderSurface.hpp>
#include <app/SwapChain.hpp>
#include <app/ImGuiRenderer.hpp>
#include <app/ui/ImGuiComponentManager.hpp>
#include <app/ui/components/HelloWorldWindow.hpp>
#include <app/ui/components/DebugStatsWindow.hpp>
#include <app/ui/components/BulletText.hpp>
#include <app/ui/components/Button.hpp>
#include <app/ui/components/Checkbox.hpp>
#include <app/ui/components/Image.hpp>
#include <app/ui/components/ImageButton.hpp>
#include <app/ui/components/LabelText.hpp>
#include <app/ui/components/RadioButton.hpp>
#include <app/ui/components/Text.hpp>
#include <app/ui/components/TextColored.hpp>
#include <app/ui/components/TextWrapped.hpp>
#include <launcher/LauncherCatalog.hpp>
#include <launcher/PolyHavenClient.hpp>

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

namespace sauce {

class SauceEngineApp {
public:
  SauceEngineApp();
  void run(const uint32_t width, const uint32_t height);
  ~SauceEngineApp();

private:
  GLFWwindow* window = nullptr;

  std::chrono::steady_clock::time_point lastFrameTime = std::chrono::steady_clock::now();
  float deltaTime = 0.0f;

  float lastX = 0.0f;
  float lastY = 0.0f;
  bool firstMouse = true;
  bool cursorCaptured = true;
  bool gravePressedLastFrame = false;

  std::unique_ptr<sauce::Instance> pInstance;
  std::unique_ptr<sauce::RenderSurface> pRenderSurface;

  sauce::PhysicalDevice physicalDevice = nullptr;
  sauce::LogicalDevice logicalDevice = nullptr;

  std::unique_ptr<sauce::Renderer> pRenderer;
  std::unique_ptr<sauce::Scene> pScene;
  std::unique_ptr<sauce::ImGuiRenderer> pImGuiRenderer;

  std::unique_ptr<sauce::ui::ImGuiComponentManager> pImGuiComponentManager;
  std::function<void(sauce::ui::ImGuiComponentManager&)> pCustomUIBuilder;

  void initVulkan();
  void initWindow();
  void mainLoop();
  void processInput(float deltaTime);
  static void mouseCallback(GLFWwindow* window, double xposIn, double yposIn);

  void buildExampleUI();
  void renderLauncherUI();
  void pollLauncherBackgroundTasks();
  void updateLauncherPreview();
  void refreshLauncherCacheState();
  bool launchFromLauncher();
  bool finalizeLauncherLaunch(const std::string& resolvedScenePath, const std::string& resolvedIblPath);
  bool loadConfiguredScene();
  bool resolveConfiguredRemoteAssets(std::string& errorMessage);
  void setCursorCapture(bool captured);

  void uploadMeshGPUResources();
  void setupSceneRenderer();
  void syncRigidBodiesToTransforms();
  void recordSceneCommandBuffer(vk::raii::CommandBuffer& cmd, uint32_t imageIndex);

public:
  sauce::ui::ImGuiComponentManager& getImGuiManager() { return *pImGuiComponentManager; }
  void setCustomUIBuilder(std::function<void(sauce::ui::ImGuiComponentManager&)> builder);
  void setSceneFile(const std::string& path) { sceneFile = path; }
  void setIBLFile(const std::string& path) { iblFile = path; }
  void setPolyHavenModelSelection(const std::string& id, const std::string& resolution) { polyHavenModelId = id; polyHavenModelResolution = resolution; }
  void setPolyHavenHdriSelection(const std::string& id, const std::string& resolution) { polyHavenHdriId = id; polyHavenHdriResolution = resolution; }
  void setLauncherEnabled(bool enabled) { launcherEnabled = enabled; launcherActive = enabled; }

private:
  struct RemoteBrowserState {
    std::vector<sauce::launcher::PolyHavenAssetSummary> assets;
    std::string searchQuery;
    std::string statusMessage;
    std::optional<std::future<sauce::launcher::PolyHavenAssetListResult>> pendingLoad;
    int selectedIndex = -1;
    int selectedResolutionIndex = 0;
    int selectedSortMode = 1;
  };

  struct PendingLaunchResult {
    std::string scenePath;
    std::string iblPath;
    std::string errorMessage;
  };

  struct LauncherSelectionState {
    struct CacheState {
      sauce::launcher::PolyHavenCacheSummary summary;
      std::string statusMessage;
      int pruneDays = 30;
    };

    sauce::launcher::AssetCatalog catalog;
    int selectedLaunchTarget = 0;
    int selectedIblMap = 0;
    int selectedResolutionPreset = 0;
    int selectedSceneTab = 0;
    int selectedIblTab = 0;
    std::string scenePath;
    std::string iblPath;
    uint32_t launchWidth = AppOptions::DEFAULT_SCR_WIDTH;
    uint32_t launchHeight = AppOptions::DEFAULT_SCR_HEIGHT;
    std::string commandPreview;
    std::string statusMessage;
    RemoteBrowserState remoteModels;
    RemoteBrowserState remoteHdris;
    CacheState cache;
    std::optional<std::future<PendingLaunchResult>> pendingLaunch;
  };

  std::string sceneFile;
  std::string iblFile;
  std::string polyHavenModelId;
  std::string polyHavenModelResolution = "2k";
  std::string polyHavenHdriId;
  std::string polyHavenHdriResolution = "4k";
  bool launcherEnabled = false;
  bool launcherActive = false;
  LauncherSelectionState launcherState;
  uint32_t width = 0;
  uint32_t height = 0;
};

} // namespace sauce
