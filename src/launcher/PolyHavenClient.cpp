#include <launcher/PolyHavenClient.hpp>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <map>
#include <mutex>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace sauce::launcher {

namespace fs = std::filesystem;

namespace {

constexpr const char* kApiBaseUrl = "https://api.polyhaven.com";
constexpr const char* kUserAgent = "SauceEngineLauncher/0.1 (+https://github.com/aaron/SauceEngine)";

using json = nlohmann::json;

struct RemoteFile {
  std::string url;
  std::string relativePath;
};

size_t writeStringCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
  auto* out = static_cast<std::string*>(userdata);
  out->append(ptr, size * nmemb);
  return size * nmemb;
}

size_t writeFileCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
  auto* out = static_cast<std::ofstream*>(userdata);
  out->write(ptr, static_cast<std::streamsize>(size * nmemb));
  return size * nmemb;
}

void ensureCurlInitialized() {
  static std::once_flag once;
  std::call_once(once, []() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
  });
}

std::string httpGetText(const std::string& url) {
  ensureCurlInitialized();

  CURL* curl = curl_easy_init();
  if (!curl) {
    throw std::runtime_error("Failed to initialize curl");
  }

  std::string response;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERAGENT, kUserAgent);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStringCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  const CURLcode code = curl_easy_perform(curl);
  if (code != CURLE_OK) {
    const std::string error = curl_easy_strerror(code);
    curl_easy_cleanup(curl);
    throw std::runtime_error("HTTP request failed: " + error);
  }

  curl_easy_cleanup(curl);
  return response;
}

void downloadFile(const std::string& url, const fs::path& destination) {
  ensureCurlInitialized();

  fs::create_directories(destination.parent_path());
  const fs::path tempPath = destination.string() + ".part";

  std::ofstream output(tempPath, std::ios::binary);
  if (!output.is_open()) {
    throw std::runtime_error("Failed to open download target: " + destination.string());
  }

  CURL* curl = curl_easy_init();
  if (!curl) {
    throw std::runtime_error("Failed to initialize curl");
  }

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_USERAGENT, kUserAgent);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFileCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);

  const CURLcode code = curl_easy_perform(curl);
  output.close();

  if (code != CURLE_OK) {
    fs::remove(tempPath);
    const std::string error = curl_easy_strerror(code);
    curl_easy_cleanup(curl);
    throw std::runtime_error("Download failed: " + error);
  }

  curl_easy_cleanup(curl);
  fs::rename(tempPath, destination);
}

std::string assetTypeQueryValue(PolyHavenAssetType type) {
  switch (type) {
    case PolyHavenAssetType::Hdri:
      return "hdris";
    case PolyHavenAssetType::Model:
      return "models";
  }
  return "hdris";
}

int resolutionRank(const std::string& resolution) {
  static const std::map<std::string, int> ranks = {
      {"1k", 1},
      {"2k", 2},
      {"4k", 4},
      {"8k", 8},
      {"16k", 16},
  };
  const auto it = ranks.find(resolution);
  if (it == ranks.end()) {
    return 0;
  }
  return it->second;
}

std::string pickBestResolution(const json& filesNode, const std::string& preferredResolution) {
  if (filesNode.contains(preferredResolution)) {
    return preferredResolution;
  }

  std::vector<std::string> available;
  available.reserve(filesNode.size());
  for (const auto& [key, _] : filesNode.items()) {
    available.push_back(key);
  }
  if (available.empty()) {
    return {};
  }

  std::ranges::sort(available, [](const std::string& lhs, const std::string& rhs) {
    return resolutionRank(lhs) < resolutionRank(rhs);
  });

  const int preferredRank = resolutionRank(preferredResolution);
  for (const std::string& candidate : available) {
    if (resolutionRank(candidate) >= preferredRank) {
      return candidate;
    }
  }
  return available.back();
}

std::string formatMaxResolutionLabel(const json& maxResolution) {
  if (!maxResolution.is_array() || maxResolution.size() < 2) {
    return {};
  }

  const int width = maxResolution[0].get<int>();
  if (width >= 16000) {
    return "16k";
  }
  if (width >= 8000) {
    return "8k";
  }
  if (width >= 4000) {
    return "4k";
  }
  if (width >= 2000) {
    return "2k";
  }
  return "1k";
}

std::string pathBasenameFromUrl(const std::string& url) {
  const size_t slash = url.find_last_of('/');
  if (slash == std::string::npos) {
    return url;
  }
  return url.substr(slash + 1);
}

std::vector<std::string> jsonStringArray(const json& arrayNode) {
  std::vector<std::string> values;
  if (!arrayNode.is_array()) {
    return values;
  }

  values.reserve(arrayNode.size());
  for (const auto& value : arrayNode) {
    if (value.is_string()) {
      values.push_back(value.get<std::string>());
    }
  }
  return values;
}

PolyHavenAssetSummary parseAssetSummary(const std::string& id, const json& node) {
  PolyHavenAssetSummary summary;
  summary.id = id;
  summary.name = node.value("name", id);
  summary.description = node.value("description", "");
  summary.thumbnailUrl = node.value("thumbnail_url", "");
  summary.downloadCount = node.value("download_count", 0);
  summary.categories = jsonStringArray(node.value("categories", json::array()));
  summary.tags = jsonStringArray(node.value("tags", json::array()));
  summary.maxResolutionLabel = formatMaxResolutionLabel(node.value("max_resolution", json::array()));
  return summary;
}

std::vector<RemoteFile> hdriFileFromInfo(const json& filesNode, const std::string& id, const std::string& preferredResolution) {
  const json& hdriNode = filesNode.at("hdri");
  const std::string resolution = pickBestResolution(hdriNode, preferredResolution);
  if (resolution.empty()) {
    throw std::runtime_error("No HDRI resolutions are available for " + id);
  }

  const json& resolutionNode = hdriNode.at(resolution);
  if (resolutionNode.contains("hdr")) {
    const std::string url = resolutionNode.at("hdr").at("url").get<std::string>();
    return {{
        .url = url,
        .relativePath = pathBasenameFromUrl(url),
    }};
  }

  throw std::runtime_error("No .hdr download is available for " + id + " at " + resolution);
}

std::vector<RemoteFile> modelFilesFromInfo(const json& filesNode, const std::string& id, const std::string& preferredResolution) {
  const json& gltfNode = filesNode.at("gltf");
  const std::string resolution = pickBestResolution(gltfNode, preferredResolution);
  if (resolution.empty()) {
    throw std::runtime_error("No glTF resolutions are available for " + id);
  }

  const json& fileNode = gltfNode.at(resolution).at("gltf");
  std::vector<RemoteFile> files;
  const std::string rootUrl = fileNode.at("url").get<std::string>();
  files.push_back({
      .url = rootUrl,
      .relativePath = pathBasenameFromUrl(rootUrl),
  });

  if (fileNode.contains("include")) {
    for (const auto& [relativePath, includeNode] : fileNode.at("include").items()) {
      files.push_back({
          .url = includeNode.at("url").get<std::string>(),
          .relativePath = relativePath,
      });
    }
  }

  return files;
}

fs::path userHomePath() {
  const char* home = std::getenv("HOME");
  if (home != nullptr && *home != '\0') {
    return fs::path(home);
  }
#if defined(_WIN32)
  const char* userProfile = std::getenv("USERPROFILE");
  if (userProfile != nullptr && *userProfile != '\0') {
    return fs::path(userProfile);
  }
#endif
  return fs::temp_directory_path();
}

fs::path cacheTypeRoot(PolyHavenAssetType type) {
  switch (type) {
    case PolyHavenAssetType::Hdri:
      return getPolyHavenCacheRoot() / "hdris";
    case PolyHavenAssetType::Model:
      return getPolyHavenCacheRoot() / "models";
  }
  return getPolyHavenCacheRoot();
}

uintmax_t directorySize(const fs::path& root) {
  std::error_code error;
  if (!fs::exists(root, error)) {
    return 0;
  }

  uintmax_t totalBytes = 0;
  for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, error), end;
       it != end;
       it.increment(error)) {
    if (error) {
      error.clear();
      continue;
    }
    if (it->is_regular_file(error)) {
      totalBytes += it->file_size(error);
      error.clear();
    }
  }
  return totalBytes;
}

size_t directoryFileCount(const fs::path& root) {
  std::error_code error;
  if (!fs::exists(root, error)) {
    return 0;
  }

  size_t count = 0;
  for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, error), end;
       it != end;
       it.increment(error)) {
    if (error) {
      error.clear();
      continue;
    }
    if (it->is_regular_file(error)) {
      ++count;
      error.clear();
    }
  }
  return count;
}

fs::file_time_type newestWriteTime(const fs::path& root) {
  std::error_code error;
  auto newest = (fs::file_time_type::min)();
  if (!fs::exists(root, error)) {
    return newest;
  }

  newest = fs::last_write_time(root, error);
  if (error) {
    error.clear();
    newest = (fs::file_time_type::min)();
  }

  for (fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, error), end;
       it != end;
       it.increment(error)) {
    if (error) {
      error.clear();
      continue;
    }

    const auto candidate = it->last_write_time(error);
    if (!error && candidate > newest) {
      newest = candidate;
    }
    error.clear();
  }

  return newest;
}

void collectCachedAssetIds(const fs::path& root, std::vector<std::string>& ids, size_t& assetCount) {
  std::error_code error;
  if (!fs::exists(root, error)) {
    return;
  }

  for (fs::directory_iterator it(root, fs::directory_options::skip_permission_denied, error), end;
       it != end;
       it.increment(error)) {
    if (error) {
      error.clear();
      continue;
    }
    if (!it->is_directory(error)) {
      error.clear();
      continue;
    }
    ids.push_back(it->path().filename().string());
    ++assetCount;
    error.clear();
  }

  std::ranges::sort(ids);
}

PolyHavenCacheMutationResult removeDirectoryTree(const fs::path& directory) {
  PolyHavenCacheMutationResult result;
  std::error_code error;
  if (!fs::exists(directory, error)) {
    return result;
  }

  result.bytesRemoved = directorySize(directory);
  result.assetsRemoved = 1;
  const auto removedCount = fs::remove_all(directory, error);
  if (error) {
    result.errorMessage = "Failed to remove cache entry: " + error.message();
    result.bytesRemoved = 0;
    result.assetsRemoved = 0;
    return result;
  }
  if (removedCount == 0) {
    result.assetsRemoved = 0;
  }
  return result;
}

template <typename FilesBuilder>
PolyHavenDownloadResult downloadAssetFiles(const std::string& id,
                                           const std::string& preferredResolution,
                                           const fs::path& assetRoot,
                                           FilesBuilder&& filesBuilder) {
  try {
    const json filesNode = json::parse(httpGetText(std::string(kApiBaseUrl) + "/files/" + id));
    const std::vector<RemoteFile> files = filesBuilder(filesNode, id, preferredResolution);
    if (files.empty()) {
      throw std::runtime_error("No downloadable files were returned for " + id);
    }

    for (const RemoteFile& file : files) {
      const fs::path destination = assetRoot / file.relativePath;
      if (fs::exists(destination) && fs::file_size(destination) > 0) {
        continue;
      }
      downloadFile(file.url, destination);
    }

    return {.localPath = assetRoot / files.front().relativePath, .errorMessage = {}};
  } catch (const std::exception& error) {
    return {.localPath = {}, .errorMessage = error.what()};
  }
}

} // namespace

std::filesystem::path getPolyHavenCacheRoot() {
  const fs::path home = userHomePath();
#if defined(__APPLE__)
  return home / "Library" / "Caches" / "SauceEngine" / "polyhaven";
#elif defined(_WIN32)
  if (const char* localAppData = std::getenv("LOCALAPPDATA"); localAppData != nullptr && *localAppData != '\0') {
    return fs::path(localAppData) / "SauceEngine" / "polyhaven";
  }
  if (const char* appData = std::getenv("APPDATA"); appData != nullptr && *appData != '\0') {
    return fs::path(appData) / "SauceEngine" / "polyhaven";
  }
  return home / "AppData" / "Local" / "SauceEngine" / "polyhaven";
#else
  if (const char* xdg = std::getenv("XDG_CACHE_HOME"); xdg != nullptr && *xdg != '\0') {
    return fs::path(xdg) / "SauceEngine" / "polyhaven";
  }
  return home / ".cache" / "SauceEngine" / "polyhaven";
#endif
}

std::filesystem::path getPolyHavenHdriCachePath(const std::string& id, const std::string& resolutionLabel) {
  return getPolyHavenCacheRoot() / "hdris" / id / resolutionLabel / (id + "_" + resolutionLabel + ".hdr");
}

std::filesystem::path getPolyHavenModelCachePath(const std::string& id, const std::string& resolutionLabel) {
  return getPolyHavenCacheRoot() / "models" / id / resolutionLabel / (id + "_" + resolutionLabel + ".gltf");
}

PolyHavenAssetListResult fetchPolyHavenAssets(PolyHavenAssetType type) {
  try {
    const std::string url = std::string(kApiBaseUrl) + "/assets?type=" + assetTypeQueryValue(type);
    const json payload = json::parse(httpGetText(url));

    PolyHavenAssetListResult result;
    result.assets.reserve(payload.size());
    for (const auto& [id, node] : payload.items()) {
      result.assets.push_back(parseAssetSummary(id, node));
    }

    std::ranges::sort(result.assets, [](const PolyHavenAssetSummary& lhs, const PolyHavenAssetSummary& rhs) {
      return lhs.name < rhs.name;
    });

    return result;
  } catch (const std::exception& error) {
    return {.assets = {}, .errorMessage = error.what()};
  }
}

PolyHavenDownloadResult downloadPolyHavenHdri(const std::string& id, const std::string& resolutionLabel) {
  const fs::path assetRoot = getPolyHavenHdriCachePath(id, resolutionLabel).parent_path();
  return downloadAssetFiles(id, resolutionLabel, assetRoot, hdriFileFromInfo);
}

PolyHavenDownloadResult downloadPolyHavenModelGltf(const std::string& id, const std::string& resolutionLabel) {
  const fs::path assetRoot = getPolyHavenModelCachePath(id, resolutionLabel).parent_path();
  return downloadAssetFiles(id, resolutionLabel, assetRoot, modelFilesFromInfo);
}

PolyHavenCacheSummary inspectPolyHavenCache() {
  PolyHavenCacheSummary summary;
  const fs::path cacheRoot = getPolyHavenCacheRoot();
  summary.totalBytes = directorySize(cacheRoot);
  summary.fileCount = directoryFileCount(cacheRoot);

  collectCachedAssetIds(cacheTypeRoot(PolyHavenAssetType::Model), summary.cachedModelIds, summary.modelAssetCount);
  collectCachedAssetIds(cacheTypeRoot(PolyHavenAssetType::Hdri), summary.cachedHdriIds, summary.hdriAssetCount);
  return summary;
}

PolyHavenCacheMutationResult removePolyHavenCachedAsset(PolyHavenAssetType type, const std::string& id) {
  return removeDirectoryTree(cacheTypeRoot(type) / id);
}

PolyHavenCacheMutationResult clearPolyHavenCache() {
  PolyHavenCacheMutationResult result;
  const fs::path cacheRoot = getPolyHavenCacheRoot();
  std::error_code error;
  if (!fs::exists(cacheRoot, error)) {
    return result;
  }

  result.bytesRemoved = directorySize(cacheRoot);
  const auto removedCount = fs::remove_all(cacheRoot, error);
  if (error) {
    result.errorMessage = "Failed to clear Poly Haven cache: " + error.message();
    result.bytesRemoved = 0;
    return result;
  }

  result.assetsRemoved = removedCount > 0 ? 1 : 0;
  return result;
}

PolyHavenCacheMutationResult prunePolyHavenCacheOlderThan(std::chrono::hours maxAge) {
  PolyHavenCacheMutationResult result;
  const auto cutoff = fs::file_time_type::clock::now() - maxAge;

  for (const PolyHavenAssetType type : {PolyHavenAssetType::Model, PolyHavenAssetType::Hdri}) {
    const fs::path root = cacheTypeRoot(type);
    std::error_code error;
    if (!fs::exists(root, error)) {
      continue;
    }

    for (fs::directory_iterator it(root, fs::directory_options::skip_permission_denied, error), end;
         it != end;
         it.increment(error)) {
      if (error) {
        result.errorMessage = "Failed to scan cache: " + error.message();
        return result;
      }
      if (!it->is_directory(error)) {
        error.clear();
        continue;
      }

      const fs::path assetDirectory = it->path();
      if (newestWriteTime(assetDirectory) >= cutoff) {
        continue;
      }

      PolyHavenCacheMutationResult removed = removeDirectoryTree(assetDirectory);
      if (!removed.errorMessage.empty()) {
        return removed;
      }
      result.bytesRemoved += removed.bytesRemoved;
      result.assetsRemoved += removed.assetsRemoved;
    }
  }

  return result;
}

} // namespace sauce::launcher
