#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <future>
#include <optional>
#include <string>
#include <vector>

#include <launcher/LauncherCatalog.hpp>
#include <launcher/PolyHavenClient.hpp>
#include <launcher/optionParser.hpp>

namespace sauce::launcher {

struct LaunchRequest {
  uint32_t width = AppOptions::DEFAULT_SCR_WIDTH;
  uint32_t height = AppOptions::DEFAULT_SCR_HEIGHT;
  std::array<float, 3> modelRotationDegrees{
      static_cast<float>(AppOptions::DEFAULT_MODEL_ROTATE_X_DEGREES),
      static_cast<float>(AppOptions::DEFAULT_MODEL_ROTATE_Y_DEGREES),
      static_cast<float>(AppOptions::DEFAULT_MODEL_ROTATE_Z_DEGREES)};
  std::string scenePath;
  std::string iblPath;
};

class AppLauncher {
public:
  explicit AppLauncher(std::filesystem::path assetRoot = std::filesystem::current_path());

  void initialize(
      uint32_t width,
      uint32_t height,
      const std::string& initialScenePath,
      const std::string& initialIblPath,
      const std::string& initialPolyHavenModelId,
      const std::string& initialPolyHavenModelResolution,
      const std::string& initialPolyHavenHdriId,
      const std::string& initialPolyHavenHdriResolution);

  void pollBackgroundTasks(const std::function<bool(const LaunchRequest&)>& finalizeLaunch);
  void render(
      const std::function<bool(const LaunchRequest&)>& finalizeLaunch,
      const std::function<void()>& requestQuit);

private:
  struct RemoteBrowserState {
    std::vector<PolyHavenAssetSummary> assets;
    std::string searchQuery;
    std::string statusMessage;
    std::optional<std::future<PolyHavenAssetListResult>> pendingLoad;
    int selectedIndex = -1;
    int selectedResolutionIndex = 0;
    int selectedSortMode = 1;
  };

  struct PendingLaunchResult {
    std::string scenePath;
    std::string iblPath;
    std::string errorMessage;
  };

  struct CacheState {
    PolyHavenCacheSummary summary;
    std::string statusMessage;
    int pruneDays = 30;
  };

  struct LauncherSelectionState {
    AssetCatalog catalog;
    int selectedLaunchTarget = 0;
    int selectedIblMap = 0;
    int selectedResolutionPreset = 0;
    int selectedSceneTab = 0;
    int selectedIblTab = 0;
    std::string scenePath;
    std::string iblPath;
    uint32_t launchWidth = AppOptions::DEFAULT_SCR_WIDTH;
    uint32_t launchHeight = AppOptions::DEFAULT_SCR_HEIGHT;
    std::array<float, 3> modelRotationDegrees{
        static_cast<float>(AppOptions::DEFAULT_MODEL_ROTATE_X_DEGREES),
        static_cast<float>(AppOptions::DEFAULT_MODEL_ROTATE_Y_DEGREES),
        static_cast<float>(AppOptions::DEFAULT_MODEL_ROTATE_Z_DEGREES)};
    bool modelRotationCustomized = false;
    std::string commandPreview;
    std::string statusMessage;
    RemoteBrowserState remoteModels;
    RemoteBrowserState remoteHdris;
    CacheState cache;
    std::optional<std::future<PendingLaunchResult>> pendingLaunch;
  };

  void updatePreview();
  void refreshCacheState();
  std::array<float, 3> suggestedModelRotationDegrees() const;
  void adoptSuggestedModelRotation(bool force);
  bool launchFromUi(const std::function<bool(const LaunchRequest&)>& finalizeLaunch);
  bool finalizeLaunchRequest(
      const std::function<bool(const LaunchRequest&)>& finalizeLaunch,
      const std::string& resolvedScenePath,
      const std::string& resolvedIblPath);

  std::filesystem::path assetRoot;
  LauncherSelectionState state;
  std::string polyHavenModelId;
  std::string polyHavenModelResolution = "2k";
  std::string polyHavenHdriId;
  std::string polyHavenHdriResolution = "4k";
};

}  // namespace sauce::launcher
