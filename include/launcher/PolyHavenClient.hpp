#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace sauce::launcher {

enum class PolyHavenAssetType {
  Hdri,
  Model,
};

struct PolyHavenAssetSummary {
  std::string id;
  std::string name;
  std::string description;
  std::string thumbnailUrl;
  std::vector<std::string> categories;
  std::vector<std::string> tags;
  int downloadCount = 0;
  std::string maxResolutionLabel;
};

struct PolyHavenAssetListResult {
  std::vector<PolyHavenAssetSummary> assets;
  std::string errorMessage;
};

struct PolyHavenDownloadResult {
  std::filesystem::path localPath;
  std::string errorMessage;
};

struct PolyHavenCacheSummary {
  uintmax_t totalBytes = 0;
  size_t fileCount = 0;
  size_t modelAssetCount = 0;
  size_t hdriAssetCount = 0;
  std::vector<std::string> cachedModelIds;
  std::vector<std::string> cachedHdriIds;
};

struct PolyHavenCacheMutationResult {
  uintmax_t bytesRemoved = 0;
  size_t assetsRemoved = 0;
  std::string errorMessage;
};

std::filesystem::path getPolyHavenCacheRoot();
std::filesystem::path getPolyHavenHdriCachePath(const std::string& id, const std::string& resolutionLabel);
std::filesystem::path getPolyHavenModelCachePath(const std::string& id, const std::string& resolutionLabel);
PolyHavenAssetListResult fetchPolyHavenAssets(PolyHavenAssetType type);
PolyHavenDownloadResult downloadPolyHavenHdri(const std::string& id, const std::string& resolutionLabel);
PolyHavenDownloadResult downloadPolyHavenModelGltf(const std::string& id, const std::string& resolutionLabel);
PolyHavenCacheSummary inspectPolyHavenCache();
PolyHavenCacheMutationResult removePolyHavenCachedAsset(PolyHavenAssetType type, const std::string& id);
PolyHavenCacheMutationResult clearPolyHavenCache();
PolyHavenCacheMutationResult prunePolyHavenCacheOlderThan(std::chrono::hours maxAge);

} // namespace sauce::launcher
