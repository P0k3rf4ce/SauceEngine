#include <launcher/AppLauncher.hpp>

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <sstream>
#include <utility>

namespace sauce::launcher {

namespace {

namespace fs = std::filesystem;

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

std::string normalizeSearchText(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    if (std::isalnum(ch)) {
      return static_cast<char>(std::tolower(ch));
    }
    return ' ';
  });
  return value;
}

std::array<float, 3> defaultModelRotationDegrees() {
  return {
      static_cast<float>(AppOptions::DEFAULT_MODEL_ROTATE_X_DEGREES),
      static_cast<float>(AppOptions::DEFAULT_MODEL_ROTATE_Y_DEGREES),
      static_cast<float>(AppOptions::DEFAULT_MODEL_ROTATE_Z_DEGREES)};
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

bool matchesRemoteSearch(const PolyHavenAssetSummary& asset, const std::string& query) {
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

int remoteResolutionRank(const PolyHavenAssetSummary& asset) {
  return launcherResolutionRank(asset.maxResolutionLabel);
}

bool pathLooksLikeAuthoredScene(const std::string& path) {
  if (path.empty()) {
    return false;
  }

  const std::string normalized = normalizeSearchText(path);
  return normalized.find("testscene") != std::string::npos;
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

const PolyHavenAssetSummary* findAssetById(
    const std::vector<PolyHavenAssetSummary>& assets,
    const std::string& id) {
  if (id.empty()) {
    return nullptr;
  }

  const auto it = std::find_if(assets.begin(), assets.end(), [&](const PolyHavenAssetSummary& asset) {
    return asset.id == id;
  });
  return it == assets.end() ? nullptr : &(*it);
}

}  // namespace

AppLauncher::AppLauncher(std::filesystem::path assetRoot)
    : assetRoot(std::move(assetRoot)) {}

void AppLauncher::initialize(
    uint32_t width,
    uint32_t height,
    const std::string& initialScenePath,
    const std::string& initialIblPath,
    const std::string& initialPolyHavenModelId,
    const std::string& initialPolyHavenModelResolution,
    const std::string& initialPolyHavenHdriId,
    const std::string& initialPolyHavenHdriResolution) {
  polyHavenModelId = initialPolyHavenModelId;
  polyHavenModelResolution = initialPolyHavenModelResolution.empty() ? "2k" : initialPolyHavenModelResolution;
  polyHavenHdriId = initialPolyHavenHdriId;
  polyHavenHdriResolution = initialPolyHavenHdriResolution.empty() ? "4k" : initialPolyHavenHdriResolution;

  state.catalog = discoverAssetCatalog(assetRoot);
  state.selectedLaunchTarget = 0;
  state.selectedIblMap = 0;
  state.launchWidth = width;
  state.launchHeight = height;
  state.selectedResolutionPreset = findResolutionPreset(width, height);
  state.selectedSceneTab = polyHavenModelId.empty() ? 0 : 1;
  state.selectedIblTab = polyHavenHdriId.empty() ? 0 : 1;
  state.scenePath = initialScenePath.empty()
      ? (state.catalog.launchTargets.empty() ? std::string() : state.catalog.launchTargets.front().path)
      : initialScenePath;
  state.iblPath = initialIblPath.empty()
      ? (state.catalog.iblMaps.empty() ? std::string() : state.catalog.iblMaps.front().path)
      : initialIblPath;
  state.remoteModels.selectedResolutionIndex =
      findStringOptionIndex(kModelResolutionOptions, polyHavenModelResolution, 1);
  state.remoteHdris.selectedResolutionIndex =
      findStringOptionIndex(kHdriResolutionOptions, polyHavenHdriResolution, 2);
  state.modelRotationDegrees = defaultModelRotationDegrees();
  state.modelRotationCustomized = false;
  adoptSuggestedModelRotation(true);
  state.statusMessage = "Pick a scene, choose lighting, then launch into the runtime.";
  refreshCacheState();
  updatePreview();
}

void AppLauncher::pollBackgroundTasks(const std::function<bool(const LaunchRequest&)>& finalizeLaunch) {
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

  pollCatalog(state.remoteModels);
  pollCatalog(state.remoteHdris);

  if (!state.pendingLaunch.has_value()) {
    return;
  }

  auto& future = *state.pendingLaunch;
  if (future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
    return;
  }

  PendingLaunchResult result = future.get();
  state.pendingLaunch.reset();
  if (!result.errorMessage.empty()) {
    state.statusMessage = result.errorMessage;
    return;
  }

  refreshCacheState();
  updatePreview();
  finalizeLaunchRequest(finalizeLaunch, result.scenePath, result.iblPath);
}

void AppLauncher::refreshCacheState() {
  state.cache.summary = inspectPolyHavenCache();
  if (state.cache.summary.totalBytes == 0) {
    state.cache.statusMessage = "Cache is empty.";
  } else {
    state.cache.statusMessage =
        formatBytes(state.cache.summary.totalBytes) + " across " +
        std::to_string(state.cache.summary.fileCount) + " files.";
  }
}

std::array<float, 3> AppLauncher::suggestedModelRotationDegrees() const {
  if (state.selectedSceneTab == 0 &&
      state.selectedLaunchTarget >= 0 &&
      state.selectedLaunchTarget < static_cast<int>(state.catalog.launchTargets.size())) {
    const auto& selectedEntry = state.catalog.launchTargets[static_cast<size_t>(state.selectedLaunchTarget)];
    if (selectedEntry.group == "Scenes") {
      return {0.0f, 0.0f, 0.0f};
    }
    return defaultModelRotationDegrees();
  }

  return pathLooksLikeAuthoredScene(state.scenePath) ? std::array<float, 3>{0.0f, 0.0f, 0.0f} : defaultModelRotationDegrees();
}

void AppLauncher::adoptSuggestedModelRotation(bool force) {
  if (!force && state.modelRotationCustomized) {
    return;
  }

  state.modelRotationDegrees = suggestedModelRotationDegrees();
}

void AppLauncher::render(
    const std::function<bool(const LaunchRequest&)>& finalizeLaunch,
    const std::function<void()>& requestQuit) {
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
  auto selectedSceneAsset = findAssetById(state.remoteModels.assets, polyHavenModelId);
  auto selectedHdriAsset = findAssetById(state.remoteHdris.assets, polyHavenHdriId);
  const bool selectedModelCached = !polyHavenModelId.empty() &&
                                   sortedContains(state.cache.summary.cachedModelIds, polyHavenModelId);
  const bool selectedHdriCached = !polyHavenHdriId.empty() &&
                                  sortedContains(state.cache.summary.cachedHdriIds, polyHavenHdriId);

  auto focusedRemoteAsset = [&]() -> const PolyHavenAssetSummary* {
    if (state.selectedSceneTab == 1 && selectedSceneAsset != nullptr) {
      return selectedSceneAsset;
    }
    if (state.selectedIblTab == 1 && selectedHdriAsset != nullptr) {
      return selectedHdriAsset;
    }
    return selectedSceneAsset != nullptr ? selectedSceneAsset : selectedHdriAsset;
  }();

  auto describeLocalSelection = [](const std::string& value, const char* fallback) {
    return value.empty() ? std::string(fallback) : value;
  };
  auto applyCacheMutation = [&](const PolyHavenCacheMutationResult& result, const char* emptyMessage, const char* successPrefix) {
    if (!result.errorMessage.empty()) {
      state.cache.statusMessage = result.errorMessage;
      return;
    }

    refreshCacheState();
    if (result.assetsRemoved == 0 && result.bytesRemoved == 0) {
      state.cache.statusMessage = emptyMessage;
    } else {
      state.cache.statusMessage =
          std::string(successPrefix) + " Freed " + formatBytes(result.bytesRemoved) + ".";
    }
    updatePreview();
  };

  ImGui::TextColored(theme.muted, "SauceEngine");
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
  ImGui::SetWindowFontScale(1.35f);
  ImGui::TextUnformatted("Launcher");
  ImGui::SetWindowFontScale(1.0f);
  ImGui::TextColored(
      ImVec4(0.74f, 0.84f, 0.95f, 1.0f),
      "%zu local targets  |  %zu local maps  |  %u x %u  |  %s cache",
      state.catalog.launchTargets.size(),
      state.catalog.iblMaps.size(),
      state.launchWidth,
      state.launchHeight,
      formatBytes(state.cache.summary.totalBytes).c_str());
  ImGui::Dummy(ImVec2(0.0f, 14.0f));

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
  const std::string cacheRootText = getPolyHavenCacheRoot().string();

  auto startRemoteLoadIfNeeded = [&](RemoteBrowserState& browserState, PolyHavenAssetType type) {
    if (!browserState.assets.empty() || browserState.pendingLoad.has_value() || !browserState.statusMessage.empty()) {
      return;
    }

    browserState.statusMessage = kPolyHavenLoadingMessage;
    browserState.pendingLoad = std::async(std::launch::async, [type]() {
      return fetchPolyHavenAssets(type);
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
                                   PolyHavenAssetType assetType,
                                   const char* listId,
                                   const std::vector<std::string>& cachedIds) {
    startRemoteLoadIfNeeded(browserState, assetType);

    if (ImGui::Button(refreshLabel)) {
      browserState.assets.clear();
      browserState.selectedIndex = -1;
      browserState.statusMessage = kPolyHavenLoadingMessage;
      browserState.pendingLoad = std::async(std::launch::async, [assetType]() {
        return fetchPolyHavenAssets(assetType);
      });
    }
      ImGui::SameLine();
      ImGui::SetNextItemWidth(std::min(240.0f, ImGui::GetContentRegionAvail().x * 0.40f));
    ImGui::InputTextWithHint(searchLabel, "Filter by name, tag, or category", &browserState.searchQuery);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(140.0f);
    const std::string sortLabel = std::string("Sort##") + listId;
    ImGui::Combo(sortLabel.c_str(), &browserState.selectedSortMode, kRemoteSortOptions.data(), static_cast<int>(kRemoteSortOptions.size()));

    const int safeResolutionIndex = std::clamp(resolutionIndex, 0, static_cast<int>(resolutionOptions.size()) - 1);
    const char* resolutionLabel = resolutionOptions[static_cast<size_t>(safeResolutionIndex)].label;
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::BeginCombo(presetLabel, resolutionLabel)) {
      for (int i = 0; i < static_cast<int>(resolutionOptions.size()); ++i) {
        const bool selected = resolutionIndex == i;
        if (ImGui::Selectable(resolutionOptions[static_cast<size_t>(i)].label, selected)) {
          resolutionIndex = i;
          selectedResolution = resolutionOptions[static_cast<size_t>(i)].value;
          updatePreview();
        }
        if (selected) {
          ImGui::SetItemDefaultFocus();
        }
      }
      ImGui::EndCombo();
    }

    if (!browserState.statusMessage.empty()) {
      ImGui::SameLine();
      ImGui::TextColored(theme.subtle, "%s", browserState.statusMessage.c_str());
    }
    ImGui::Dummy(ImVec2(0.0f, 8.0f));

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
      if (cached) {
        rowLabel << "  [cached]";
      }

      if (ImGui::Selectable((rowLabel.str() + "##" + asset.id).c_str(), selected)) {
        browserState.selectedIndex = i;
        selectedId = asset.id;
        selectedResolution = resolutionOptions[static_cast<size_t>(safeResolutionIndex)].value;
        localPathToClear.clear();
        selectedLocalIndex = -1;
        adoptSuggestedModelRotation(false);
        updatePreview();
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
  if (ImGui::BeginTabBar("SceneSourceTabs")) {
    if (ImGui::BeginTabItem(kLocalSceneTabLabel)) {
      state.selectedSceneTab = 0;
      ImGui::InputTextWithHint("##manualScene", "Custom glTF / GLB path", &state.scenePath);
      if (ImGui::IsItemEdited()) {
        state.selectedLaunchTarget = -1;
        polyHavenModelId.clear();
        adoptSuggestedModelRotation(false);
        updatePreview();
      }
      ImGui::Dummy(ImVec2(0.0f, 6.0f));
      ImGui::BeginChild("LaunchTargetList", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
      for (int i = 0; i < static_cast<int>(state.catalog.launchTargets.size()); ++i) {
        const auto& entry = state.catalog.launchTargets[static_cast<size_t>(i)];
        const bool selected = state.selectedLaunchTarget == i && polyHavenModelId.empty();
        const std::string rowLabel = entry.label + " [" + entry.group + "]";
        if (ImGui::Selectable((rowLabel + "##launch-target").c_str(), selected)) {
          state.selectedLaunchTarget = i;
          state.scenePath = entry.path;
          polyHavenModelId.clear();
          adoptSuggestedModelRotation(false);
          updatePreview();
        }
      }
      ImGui::EndChild();
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(kRemoteSceneTabLabel)) {
      state.selectedSceneTab = 1;
      renderRemoteAssetList(
          state.remoteModels,
          kModelResolutionOptions,
          state.remoteModels.selectedResolutionIndex,
          polyHavenModelId,
          polyHavenModelResolution,
          state.scenePath,
          state.selectedLaunchTarget,
          "##polyhaven-model-search",
          "Refresh Models",
          "Download preset##polyhaven-model-preset",
          PolyHavenAssetType::Model,
          "PolyHavenModelList",
          state.cache.summary.cachedModelIds);
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }
  endSurface();

  ImGui::Dummy(ImVec2(0.0f, theme.gap));

  beginSurface("LaunchConfigCard", ImVec2(browserWidth, browserHeight));
  renderSectionEyebrow("ENVIRONMENT / IBL", ImGui::ColorConvertU32ToFloat4(theme.success));
  if (ImGui::BeginTabBar("IblSourceTabs")) {
    if (ImGui::BeginTabItem(kLocalSceneTabLabel)) {
      state.selectedIblTab = 0;
      ImGui::InputTextWithHint("##manualIbl", "Custom HDR / environment map path", &state.iblPath);
      if (ImGui::IsItemEdited()) {
        state.selectedIblMap = -1;
        polyHavenHdriId.clear();
        updatePreview();
      }
      ImGui::Dummy(ImVec2(0.0f, 6.0f));
      ImGui::BeginChild("IblList", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders);
      for (int i = 0; i < static_cast<int>(state.catalog.iblMaps.size()); ++i) {
        const auto& entry = state.catalog.iblMaps[static_cast<size_t>(i)];
        const bool selected = state.selectedIblMap == i && polyHavenHdriId.empty();
        const std::string rowLabel = entry.label + " [" + entry.group + "]";
        if (ImGui::Selectable((rowLabel + "##ibl-target").c_str(), selected)) {
          state.selectedIblMap = i;
          state.iblPath = entry.path;
          polyHavenHdriId.clear();
          updatePreview();
        }
      }
      ImGui::EndChild();
      ImGui::EndTabItem();
    }

    if (ImGui::BeginTabItem(kRemoteSceneTabLabel)) {
      state.selectedIblTab = 1;
      renderRemoteAssetList(
          state.remoteHdris,
          kHdriResolutionOptions,
          state.remoteHdris.selectedResolutionIndex,
          polyHavenHdriId,
          polyHavenHdriResolution,
          state.iblPath,
          state.selectedIblMap,
          "##polyhaven-hdri-search",
          "Refresh HDRIs",
          "Download preset##polyhaven-hdri-preset",
          PolyHavenAssetType::Hdri,
          "PolyHavenHdriList",
          state.cache.summary.cachedHdriIds);
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

  const float sidebarPanelWidth = stackedLayout ? fullWidth : sidebarWidth;
  beginSurface("LaunchSidebarPanel", ImVec2(sidebarPanelWidth, sidebarHeight));
  renderSectionEyebrow("LAUNCH / OPTIONS", ImGui::ColorConvertU32ToFloat4(theme.accentSoft));

  ImGui::TextColored(theme.subtle, "Scene");
  if (polyHavenModelId.empty()) {
    ImGui::TextWrapped("%s", describeLocalSelection(state.scenePath, kDefaultLauncherScene).c_str());
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

  ImGui::Dummy(ImVec2(0.0f, 10.0f));
  ImGui::TextColored(theme.subtle, "Environment");
  if (polyHavenHdriId.empty()) {
    ImGui::TextWrapped("%s", state.iblPath.empty() ? "Engine default lighting" : state.iblPath.c_str());
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

  ImGui::Dummy(ImVec2(0.0f, 14.0f));
  ImGui::Separator();
  ImGui::Dummy(ImVec2(0.0f, 12.0f));

  ImGui::TextUnformatted("Model rotation");
  float rotationDegrees[3] = {
      state.modelRotationDegrees[0],
      state.modelRotationDegrees[1],
      state.modelRotationDegrees[2]};
  ImGui::SetNextItemWidth(-1.0f);
  if (ImGui::InputFloat3("##model-rotation", rotationDegrees, "%.0f deg")) {
    state.modelRotationDegrees = {rotationDegrees[0], rotationDegrees[1], rotationDegrees[2]};
    state.modelRotationCustomized = true;
    updatePreview();
  }
  if (ImGui::Button("Auto")) {
    state.modelRotationCustomized = false;
    adoptSuggestedModelRotation(true);
    updatePreview();
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset")) {
    state.modelRotationDegrees = {0.0f, 0.0f, 0.0f};
    state.modelRotationCustomized = true;
    updatePreview();
  }
  ImGui::SameLine();
  if (ImGui::Button("Y 90")) {
    state.modelRotationDegrees = {0.0f, 90.0f, 0.0f};
    state.modelRotationCustomized = true;
    updatePreview();
  }
  ImGui::TextDisabled("Auto uses X 0, Y 90, Z 0 for standalone models.");

  ImGui::Dummy(ImVec2(0.0f, 10.0f));
  const char* resolutionLabel =
      state.selectedResolutionPreset >= 0
          ? kLauncherResolutionOptions[static_cast<size_t>(state.selectedResolutionPreset)].label
          : "Custom";
  if (ImGui::BeginCombo("Resolution", resolutionLabel)) {
    for (int i = 0; i < static_cast<int>(kLauncherResolutionOptions.size()); ++i) {
      const bool selected = state.selectedResolutionPreset == i;
      const auto& preset = kLauncherResolutionOptions[static_cast<size_t>(i)];
      if (ImGui::Selectable(preset.label, selected)) {
        state.selectedResolutionPreset = i;
        state.launchWidth = preset.width;
        state.launchHeight = preset.height;
        updatePreview();
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    const bool customSelected = state.selectedResolutionPreset < 0;
    if (ImGui::Selectable("Custom", customSelected)) {
      state.selectedResolutionPreset = -1;
    }
    ImGui::EndCombo();
  }

  int resolutionValues[2] = {
      static_cast<int>(state.launchWidth),
      static_cast<int>(state.launchHeight),
  };
  if (ImGui::InputInt2("Window", resolutionValues)) {
    state.launchWidth = static_cast<uint32_t>(std::clamp(resolutionValues[0], 640, 7680));
    state.launchHeight = static_cast<uint32_t>(std::clamp(resolutionValues[1], 360, 4320));
    state.selectedResolutionPreset =
        findResolutionPreset(state.launchWidth, state.launchHeight);
    updatePreview();
  }
  ImGui::TextDisabled("Applied when leaving the launcher.");

  ImGui::Dummy(ImVec2(0.0f, 14.0f));
  ImGui::Separator();
  ImGui::Dummy(ImVec2(0.0f, 12.0f));

  renderSectionEyebrow("REMOTE DETAILS", ImGui::ColorConvertU32ToFloat4(theme.success));
  if (focusedRemoteAsset != nullptr) {
    ImGui::TextUnformatted(focusedRemoteAsset->name.c_str());
    ImGui::TextDisabled(
        "%s  |  %s  |  %d downloads",
        focusedRemoteAsset->id.c_str(),
        focusedRemoteAsset->maxResolutionLabel.empty() ? "max res ?" : focusedRemoteAsset->maxResolutionLabel.c_str(),
        focusedRemoteAsset->downloadCount);
    if (!focusedRemoteAsset->categories.empty()) {
      ImGui::Dummy(ImVec2(0.0f, 4.0f));
      ImGui::TextDisabled("Categories: %s", joinStrings(focusedRemoteAsset->categories, ", ").c_str());
    }
    if (!focusedRemoteAsset->tags.empty()) {
      ImGui::TextDisabled("Tags: %s", joinStrings(focusedRemoteAsset->tags, ", ").c_str());
    }
    if (!focusedRemoteAsset->description.empty()) {
      ImGui::Dummy(ImVec2(0.0f, 8.0f));
      ImGui::BeginChild("RemoteAssetDescription", ImVec2(0.0f, 94.0f), ImGuiChildFlags_Borders);
      ImGui::TextWrapped("%s", focusedRemoteAsset->description.c_str());
      ImGui::EndChild();
    }
  } else {
    ImGui::TextDisabled("Select a Poly Haven model or HDRI to inspect it.");
  }

  ImGui::Dummy(ImVec2(0.0f, 14.0f));
  ImGui::Separator();
  ImGui::Dummy(ImVec2(0.0f, 12.0f));

  renderSectionEyebrow("CACHE / CLI", ImGui::ColorConvertU32ToFloat4(theme.accent));
  ImGui::TextColored(theme.subtle, "%s", state.cache.statusMessage.c_str());
  ImGui::TextDisabled(
      "%zu models  |  %zu HDRIs  |  %zu files",
      state.cache.summary.modelAssetCount,
      state.cache.summary.hdriAssetCount,
      state.cache.summary.fileCount);

  state.cache.pruneDays = std::clamp(state.cache.pruneDays, 1, 365);
  if (ImGui::InputInt("Prune after days", &state.cache.pruneDays)) {
    state.cache.pruneDays = std::clamp(state.cache.pruneDays, 1, 365);
  }

  if (ImGui::Button("Refresh cache")) {
    refreshCacheState();
  }
  ImGui::SameLine();
  if (ImGui::Button("Prune stale")) {
    applyCacheMutation(
        prunePolyHavenCacheOlderThan(std::chrono::hours(24 * state.cache.pruneDays)),
        "No stale cached assets found.",
        "Pruned stale assets.");
  }
  if (ImGui::Button("Clear cache")) {
    applyCacheMutation(
        clearPolyHavenCache(),
        "Cache was already empty.",
        "Cleared Poly Haven cache.");
  }

  bool renderedCacheAction = false;
  if (selectedModelCached || selectedHdriCached) {
    ImGui::Dummy(ImVec2(0.0f, 6.0f));
  }
  if (selectedModelCached) {
    if (ImGui::Button("Remove model")) {
      applyCacheMutation(
          removePolyHavenCachedAsset(PolyHavenAssetType::Model, polyHavenModelId),
          "Selected model was not cached.",
          "Removed cached model.");
    }
    renderedCacheAction = true;
  }
  if (selectedHdriCached) {
    if (renderedCacheAction) {
      ImGui::SameLine();
    }
    if (ImGui::Button("Remove HDRI")) {
      applyCacheMutation(
          removePolyHavenCachedAsset(PolyHavenAssetType::Hdri, polyHavenHdriId),
          "Selected HDRI was not cached.",
          "Removed cached HDRI.");
    }
  }

  ImGui::TextDisabled("%s", cacheRootText.c_str());

  ImGui::Dummy(ImVec2(0.0f, 10.0f));
  ImGui::TextUnformatted("CLI Preview");
  ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::ColorConvertU32ToFloat4(theme.frameAlt));
  ImGui::InputTextMultiline("##command-preview", &state.commandPreview, ImVec2(-1.0f, 96.0f), ImGuiInputTextFlags_ReadOnly);
  ImGui::PopStyleColor();
  endSurface();

  ImGui::Dummy(ImVec2(0.0f, theme.gap));
  const bool launchBusy = state.pendingLaunch.has_value();
  beginSurface("LauncherFooter", ImVec2(0.0f, footerHeight), ImVec2(18.0f, 14.0f));
  ImGui::BeginGroup();
  ImGui::TextColored(
      launchBusy ? ImGui::ColorConvertU32ToFloat4(theme.accent) : theme.subtle,
      "%s",
      launchBusy ? "Downloading selected remote assets into the local cache." : state.statusMessage.c_str());
  ImGui::EndGroup();

  const float footerButtonWidth = 180.0f + 120.0f + 12.0f;
  const float footerRight = ImGui::GetWindowContentRegionMax().x;
  ImGui::SameLine();
  ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), footerRight - footerButtonWidth));
  if (launchBusy) {
    ImGui::BeginDisabled();
  }
  if (ImGui::Button(launchBusy ? "Preparing..." : "Launch Scene", ImVec2(180.0f, 42.0f))) {
    launchFromUi(finalizeLaunch);
  }
  if (launchBusy) {
    ImGui::EndDisabled();
  }
  ImGui::SameLine();
  if (ImGui::Button("Quit", ImVec2(120.0f, 42.0f))) {
    requestQuit();
  }
  endSurface();

  ImGui::PopStyleColor(kLauncherColorPushCount);
  ImGui::End();
  ImGui::PopStyleVar(2);
}

void AppLauncher::updatePreview() {
  AppOptions options;
  options.scr_width = state.launchWidth;
  options.scr_height = state.launchHeight;
  options.model_rotate_x_degrees = state.modelRotationDegrees[0];
  options.model_rotate_y_degrees = state.modelRotationDegrees[1];
  options.model_rotate_z_degrees = state.modelRotationDegrees[2];
  if (polyHavenModelId.empty()) {
    options.scene_file = state.scenePath;
  } else {
    const auto modelCachePath = getPolyHavenModelCachePath(polyHavenModelId, polyHavenModelResolution);
    if (fs::exists(modelCachePath)) {
      options.scene_file = modelCachePath.string();
    } else {
      options.polyhaven_model_id = polyHavenModelId;
      options.polyhaven_model_resolution = polyHavenModelResolution;
    }
  }

  if (polyHavenHdriId.empty()) {
    options.ibl_file = state.iblPath;
  } else {
    const auto hdriCachePath = getPolyHavenHdriCachePath(polyHavenHdriId, polyHavenHdriResolution);
    if (fs::exists(hdriCachePath)) {
      options.ibl_file = hdriCachePath.string();
    } else {
      options.polyhaven_hdri_id = polyHavenHdriId;
      options.polyhaven_hdri_resolution = polyHavenHdriResolution;
    }
  }
  options.skip_launcher = true;
  state.commandPreview = buildCommandLinePreview(options);
}

bool AppLauncher::launchFromUi(const std::function<bool(const LaunchRequest&)>& finalizeLaunch) {
  if (state.pendingLaunch.has_value()) {
    return false;
  }

  if (!polyHavenModelId.empty() || !polyHavenHdriId.empty()) {
    const std::string modelId = polyHavenModelId;
    const std::string modelResolution = polyHavenModelResolution;
    const std::string hdriId = polyHavenHdriId;
    const std::string hdriResolution = polyHavenHdriResolution;
    const std::string localScenePath = state.scenePath;
    const std::string localIblPath = state.iblPath;

    state.statusMessage = "Downloading selected Poly Haven assets...";
    state.pendingLaunch = std::async(std::launch::async, [modelId, modelResolution, hdriId, hdriResolution, localScenePath, localIblPath]() {
      PendingLaunchResult result;
      result.scenePath = localScenePath;
      result.iblPath = localIblPath;

      if (!modelId.empty()) {
        const auto modelDownload = downloadPolyHavenModelGltf(modelId, modelResolution);
        if (!modelDownload.errorMessage.empty()) {
          result.errorMessage = "Poly Haven model download failed: " + modelDownload.errorMessage;
          return result;
        }
        result.scenePath = modelDownload.localPath.string();
      }

      if (!hdriId.empty()) {
        const auto hdriDownload = downloadPolyHavenHdri(hdriId, hdriResolution);
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

  if (!state.scenePath.empty() && !fs::exists(state.scenePath)) {
    state.statusMessage = "Scene/model file not found: " + state.scenePath;
    return false;
  }

  if (!state.iblPath.empty() && !fs::exists(state.iblPath)) {
    state.statusMessage = "Environment map not found: " + state.iblPath;
    return false;
  }

  return finalizeLaunchRequest(finalizeLaunch, state.scenePath, state.iblPath);
}

bool AppLauncher::finalizeLaunchRequest(
    const std::function<bool(const LaunchRequest&)>& finalizeLaunch,
    const std::string& resolvedScenePath,
    const std::string& resolvedIblPath) {
  LaunchRequest request;
  request.width = state.launchWidth;
  request.height = state.launchHeight;
  request.modelRotationDegrees = state.modelRotationDegrees;
  request.scenePath = resolvedScenePath.empty() ? kDefaultLauncherScene : resolvedScenePath;
  request.iblPath = resolvedIblPath;

  if (!finalizeLaunch(request)) {
    state.statusMessage = "Failed to load the selected scene. Check the console for loader errors.";
    return false;
  }

  state.statusMessage = "Launching scene.";
  return true;
}

}  // namespace sauce::launcher
