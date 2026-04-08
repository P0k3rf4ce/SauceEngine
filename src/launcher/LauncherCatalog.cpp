#include <launcher/LauncherCatalog.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>
#include <sstream>
#include <string_view>

namespace sauce::launcher {

namespace fs = std::filesystem;

namespace {

std::string lowercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

bool hasExtension(const fs::path& path, const std::set<std::string>& extensions) {
  return extensions.contains(lowercase(path.extension().string()));
}

bool containsAnyToken(const std::string& value, std::initializer_list<std::string_view> tokens) {
  return std::any_of(tokens.begin(), tokens.end(), [&](std::string_view token) {
    return value.find(token) != std::string::npos;
  });
}

std::string toDisplayPath(const fs::path& rootPath, const fs::path& path) {
  std::error_code ec;
  const fs::path relativePath = fs::relative(path, rootPath, ec);
  if (!ec && !relativePath.empty()) {
    return relativePath.generic_string();
  }
  return path.generic_string();
}

std::string quoteIfNeeded(const std::string& value) {
  if (value.find_first_of(" \t\"") == std::string::npos) {
    return value;
  }

  std::string escaped;
  escaped.reserve(value.size() + 2);
  escaped.push_back('"');
  for (char ch : value) {
    if (ch == '"' || ch == '\\') {
      escaped.push_back('\\');
    }
    escaped.push_back(ch);
  }
  escaped.push_back('"');
  return escaped;
}

std::string directoryGroup(const fs::path& rootPath, const fs::path& path, const std::string& fallback) {
  std::error_code ec;
  const fs::path relativeParent = fs::relative(path.parent_path(), rootPath, ec);
  if (ec || relativeParent.empty() || relativeParent == ".") {
    return fallback;
  }
  return relativeParent.generic_string();
}

bool shouldSkipDirectory(const fs::path& path) {
  const std::string name = lowercase(path.filename().string());
  if (name.empty()) {
    return false;
  }

  if (name.front() == '.') {
    return true;
  }

  return name == "build" ||
         name == "out" ||
         name == "dist" ||
         name == "vcpkg_installed" ||
         name.starts_with("cmake-build");
}

bool looksLikeEnvironmentMap(const fs::path& rootPath, const fs::path& path) {
  const std::string extension = lowercase(path.extension().string());
  if (extension == ".hdr" || extension == ".exr") {
    return true;
  }

  const std::string normalizedPath = lowercase(toDisplayPath(rootPath, path));
  return containsAnyToken(normalizedPath, {"hdr", "ibl", "env", "sky"});
}

bool looksLikeAuthoredScene(const fs::path& rootPath, const fs::path& path) {
  const std::string normalizedPath = lowercase(toDisplayPath(rootPath, path));
  if (containsAnyToken(normalizedPath, {"assets/models", "/models/", "models/"})) {
    return false;
  }

  return containsAnyToken(normalizedPath, {"testscene", "/scene", "scene_", "_scene", "/scenes/"});
}

void appendDiscoveredMatches(AssetCatalog& catalog,
                             const fs::path& rootPath,
                             const std::set<std::string>& sceneExtensions,
                             const std::set<std::string>& iblExtensions) {
  std::error_code ec;
  if (!fs::exists(rootPath, ec) || !fs::is_directory(rootPath, ec)) {
    return;
  }

  fs::recursive_directory_iterator it(rootPath, fs::directory_options::skip_permission_denied, ec);
  const fs::recursive_directory_iterator end;
  while (!ec && it != end) {
    const fs::directory_entry entry = *it;

    if (entry.is_directory(ec)) {
      if (!ec && shouldSkipDirectory(entry.path())) {
        it.disable_recursion_pending();
      }
      ec.clear();
      it.increment(ec);
      continue;
    }

    if (ec) {
      ec.clear();
      it.increment(ec);
      continue;
    }

    if (entry.is_regular_file(ec)) {
      const fs::path path = entry.path();
      if (hasExtension(path, sceneExtensions)) {
        catalog.launchTargets.push_back(AssetEntry{
            .label = path.stem().string(),
            .description = toDisplayPath(rootPath, path),
            .path = path.generic_string(),
            .group = directoryGroup(rootPath, path, "Project Files"),
            .builtin = false,
            .authoredScene = looksLikeAuthoredScene(rootPath, path),
        });
      } else if (hasExtension(path, iblExtensions) && looksLikeEnvironmentMap(rootPath, path)) {
        catalog.iblMaps.push_back(AssetEntry{
            .label = path.stem().string(),
            .description = toDisplayPath(rootPath, path),
            .path = path.generic_string(),
            .group = directoryGroup(rootPath, path, "Project Files"),
            .builtin = false,
            .authoredScene = false,
        });
      }
    }

    ec.clear();
    it.increment(ec);
  }
}

void sortEntries(std::vector<AssetEntry>& entries) {
  std::sort(entries.begin(), entries.end(), [](const AssetEntry& lhs, const AssetEntry& rhs) {
    if (lhs.builtin != rhs.builtin) {
      return lhs.builtin > rhs.builtin;
    }
    if (lhs.group != rhs.group) {
      return lhs.group < rhs.group;
    }
    return lowercase(lhs.label) < lowercase(rhs.label);
  });
}

} // namespace

AssetCatalog discoverAssetCatalog(const fs::path& rootPath) {
  AssetCatalog catalog;

  catalog.launchTargets.push_back(AssetEntry{
      .label = "Default Cube",
      .description = "Built-in fallback scene",
      .path = "",
      .group = "Built-in",
      .builtin = true,
  });

  catalog.iblMaps.push_back(AssetEntry{
      .label = "Engine Default Lighting",
      .description = "No environment override",
      .path = "",
      .group = "Built-in",
      .builtin = true,
  });

  static const std::set<std::string> sceneExtensions = {".gltf", ".glb"};
  static const std::set<std::string> iblExtensions = {".hdr", ".exr", ".jpg", ".jpeg", ".png"};

  appendDiscoveredMatches(catalog, rootPath, sceneExtensions, iblExtensions);

  sortEntries(catalog.launchTargets);
  sortEntries(catalog.iblMaps);
  return catalog;
}

std::string buildCommandLinePreview(const AppOptions& options, const std::string& executableName) {
  std::ostringstream stream;
  stream << executableName << " --skip-launcher";

  if (options.scr_width != AppOptions::DEFAULT_SCR_WIDTH) {
    stream << " --width " << options.scr_width;
  }
  if (options.scr_height != AppOptions::DEFAULT_SCR_HEIGHT) {
    stream << " --height " << options.scr_height;
  }
  if (std::abs(options.tickrate - AppOptions::DEFAULT_TICKRATE) > 0.001) {
    stream << " --tickrate " << options.tickrate;
  }
  if (std::abs(options.model_rotate_x_degrees - AppOptions::DEFAULT_MODEL_ROTATE_X_DEGREES) > 0.001) {
    stream << " --model-rotate-x " << options.model_rotate_x_degrees;
  }
  if (std::abs(options.model_rotate_y_degrees - AppOptions::DEFAULT_MODEL_ROTATE_Y_DEGREES) > 0.001) {
    stream << " --model-rotate-y " << options.model_rotate_y_degrees;
  }
  if (std::abs(options.model_rotate_z_degrees - AppOptions::DEFAULT_MODEL_ROTATE_Z_DEGREES) > 0.001) {
    stream << " --model-rotate-z " << options.model_rotate_z_degrees;
  }
  if (!options.ibl_file.empty()) {
    stream << " --ibl " << quoteIfNeeded(options.ibl_file);
  }
  if (!options.scene_file.empty()) {
    stream << " --input-file " << quoteIfNeeded(options.scene_file);
  }
  if (!options.polyhaven_model_id.empty()) {
    stream << " --polyhaven-model " << quoteIfNeeded(options.polyhaven_model_id);
    if (!options.polyhaven_model_resolution.empty()) {
      stream << " --polyhaven-model-res " << quoteIfNeeded(options.polyhaven_model_resolution);
    }
  }
  if (!options.polyhaven_hdri_id.empty()) {
    stream << " --polyhaven-hdri " << quoteIfNeeded(options.polyhaven_hdri_id);
    if (!options.polyhaven_hdri_resolution.empty()) {
      stream << " --polyhaven-hdri-res " << quoteIfNeeded(options.polyhaven_hdri_resolution);
    }
  }

  return stream.str();
}

} // namespace sauce::launcher
