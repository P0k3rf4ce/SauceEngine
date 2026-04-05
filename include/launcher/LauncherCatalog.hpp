#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <launcher/optionParser.hpp>

namespace sauce::launcher {

struct AssetEntry {
  std::string label;
  std::string description;
  std::string path;
  std::string group;
  bool builtin = false;
};

struct AssetCatalog {
  std::vector<AssetEntry> launchTargets;
  std::vector<AssetEntry> iblMaps;
};

AssetCatalog discoverAssetCatalog(const std::filesystem::path& rootPath = std::filesystem::current_path());
std::string buildCommandLinePreview(const AppOptions& options, const std::string& executableName = "./SauceEngine");

} // namespace sauce::launcher
