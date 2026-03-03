#pragma once

#include <editor/panels/EditorPanel.hpp>
#include <filesystem>
#include <vector>

namespace sauce::editor {

class AssetBrowserPanel : public EditorPanel {
public:
  AssetBrowserPanel(EditorApp& app);
  void render() override;

  void handleFileDrop(const std::string& path);

private:
  void drawDirectoryTree(const std::filesystem::path& dir);
  void drawFileList();
  void importGLTF(const std::filesystem::path& path);

  std::filesystem::path rootPath;
  std::filesystem::path currentPath;
  std::filesystem::path selectedFile;
};

} // namespace sauce::editor
