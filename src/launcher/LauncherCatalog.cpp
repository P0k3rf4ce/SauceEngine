#include <launcher/LauncherCatalog.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <set>
#include <sstream>

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

void appendMatches(std::vector<AssetEntry>& out,
                   const fs::path& rootPath,
                   const fs::path& directory,
                   const std::string& group,
                   const std::set<std::string>& extensions,
                   bool recursive) {
  if (!fs::exists(directory) || !fs::is_directory(directory)) {
    return;
  }

  auto pushEntry = [&](const fs::directory_entry& entry) {
    if (!entry.is_regular_file()) {
      return;
    }

    const fs::path path = entry.path();
    if (!hasExtension(path, extensions)) {
      return;
    }

    out.push_back(AssetEntry{
        .label = path.stem().string(),
        .description = toDisplayPath(rootPath, path),
        .path = path.generic_string(),
        .group = group,
        .builtin = false,
    });
  };

  if (recursive) {
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
      pushEntry(entry);
    }
  } else {
    for (const auto& entry : fs::directory_iterator(directory)) {
      pushEntry(entry);
    }
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

  appendMatches(catalog.launchTargets, rootPath, rootPath / "assets" / "models", "Models", sceneExtensions, true);
  appendMatches(catalog.launchTargets, rootPath, rootPath / "testScene", "Scenes", sceneExtensions, true);

  appendMatches(catalog.iblMaps, rootPath, rootPath / "testScene", "Environment Maps", iblExtensions, false);
  appendMatches(catalog.iblMaps, rootPath, rootPath / "assets" / "hdr", "Environment Maps", iblExtensions, false);
  appendMatches(catalog.iblMaps, rootPath, rootPath / "assets" / "ibl", "Environment Maps", iblExtensions, false);

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
  if (!options.ibl_file.empty()) {
    stream << " --ibl " << quoteIfNeeded(options.ibl_file);
  }
  if (!options.scene_file.empty()) {
    stream << " --input-file " << quoteIfNeeded(options.scene_file);
  }

  return stream.str();
}

} // namespace sauce::launcher
