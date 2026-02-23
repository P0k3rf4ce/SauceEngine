#include <editor/panels/AssetBrowserPanel.hpp>
#include <editor/EditorApp.hpp>
#include <app/Scene.hpp>

#include <imgui.h>
#include <iostream>
#include <algorithm>

namespace sauce::editor {

static std::string formatFileSize(uintmax_t size) {
  if (size < 1024) return std::to_string(size) + " B";
  if (size < 1024 * 1024) return std::to_string(size / 1024) + " KB";
  return std::to_string(size / (1024 * 1024)) + " MB";
}

AssetBrowserPanel::AssetBrowserPanel(EditorApp& app)
  : EditorPanel("Asset Browser", app) {

  rootPath = std::filesystem::current_path() / "assets";
  if (!std::filesystem::exists(rootPath)) {
    rootPath = std::filesystem::current_path();
  }
  currentPath = rootPath;
}

void AssetBrowserPanel::render() {
  ImGui::Begin(title.c_str(), &isOpen);

  // Breadcrumb navigation
  {
    if (currentPath != rootPath) {
      if (ImGui::Button("Up")) {
        currentPath = currentPath.parent_path();
        if (!currentPath.string().starts_with(rootPath.string())) {
          currentPath = rootPath;
        }
      }
      ImGui::SameLine();
    }

    if (ImGui::Button("Refresh")) {
      // Just re-entering the directory triggers re-read
    }
    ImGui::SameLine();

    auto relativePath = std::filesystem::relative(currentPath, rootPath);
    ImGui::TextDisabled("Root /");
    ImGui::SameLine(0, 0);
    ImGui::Text(" %s", relativePath.string().c_str());
  }

  ImGui::Separator();

  // Two-column layout
  float treeWidth = ImGui::GetContentRegionAvail().x * 0.28f;

  // Directory tree (left)
  ImGui::BeginChild("DirTree", ImVec2(treeWidth, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeX);
  if (std::filesystem::exists(rootPath)) {
    drawDirectoryTree(rootPath);
  }
  ImGui::EndChild();

  ImGui::SameLine();

  // File list (right)
  ImGui::BeginChild("FileList", ImVec2(0, 0), ImGuiChildFlags_Borders);
  drawFileList();
  ImGui::EndChild();

  ImGui::End();
}

void AssetBrowserPanel::drawDirectoryTree(const std::filesystem::path& dir) {
  try {
    // Collect and sort directories
    std::vector<std::filesystem::directory_entry> dirs;
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
      if (entry.is_directory() && entry.path().filename().string()[0] != '.') {
        dirs.push_back(entry);
      }
    }
    std::sort(dirs.begin(), dirs.end(), [](const auto& a, const auto& b) {
      return a.path().filename() < b.path().filename();
    });

    for (const auto& entry : dirs) {
      std::string name = entry.path().filename().string();

      ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

      bool hasSubDirs = false;
      try {
        for (const auto& sub : std::filesystem::directory_iterator(entry.path())) {
          if (sub.is_directory() && sub.path().filename().string()[0] != '.') {
            hasSubDirs = true;
            break;
          }
        }
      } catch (...) {}

      if (!hasSubDirs) {
        flags |= ImGuiTreeNodeFlags_Leaf;
      }

      if (currentPath == entry.path()) {
        flags |= ImGuiTreeNodeFlags_Selected;
      }

      bool nodeOpen = ImGui::TreeNodeEx(name.c_str(), flags);

      if (ImGui::IsItemClicked()) {
        currentPath = entry.path();
      }

      if (nodeOpen) {
        drawDirectoryTree(entry.path());
        ImGui::TreePop();
      }
    }
  } catch (const std::filesystem::filesystem_error&) {
    ImGui::TextDisabled("Access denied");
  }
}

void AssetBrowserPanel::drawFileList() {
  if (!std::filesystem::exists(currentPath)) {
    ImGui::TextDisabled("Directory not found");
    return;
  }

  try {
    // Collect entries
    std::vector<std::filesystem::directory_entry> dirs;
    std::vector<std::filesystem::directory_entry> files;

    for (const auto& entry : std::filesystem::directory_iterator(currentPath)) {
      if (entry.path().filename().string()[0] == '.') continue; // skip hidden
      if (entry.is_directory()) {
        dirs.push_back(entry);
      } else if (entry.is_regular_file()) {
        files.push_back(entry);
      }
    }

    std::sort(dirs.begin(), dirs.end(), [](const auto& a, const auto& b) {
      return a.path().filename() < b.path().filename();
    });
    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) {
      return a.path().filename() < b.path().filename();
    });

    // Table layout
    if (ImGui::BeginTable("FileTable", 2,
        ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody)) {

      ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
      ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 70.0f);
      ImGui::TableHeadersRow();

      // Directories first
      for (const auto& entry : dirs) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        std::string name = entry.path().filename().string();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.9f, 1.0f));
        if (ImGui::Selectable(name.c_str(), false,
            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
          if (ImGui::IsMouseDoubleClicked(0)) {
            currentPath = entry.path();
          }
        }
        ImGui::PopStyleColor();

        ImGui::TableSetColumnIndex(1);
        ImGui::TextDisabled("DIR");
      }

      // Then files
      for (const auto& entry : files) {
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        std::string name = entry.path().filename().string();
        std::string ext = entry.path().extension().string();
        bool isGltf = (ext == ".gltf" || ext == ".glb");
        bool isImage = (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga");
        bool isShader = (ext == ".slang" || ext == ".spv" || ext == ".glsl" || ext == ".vert" || ext == ".frag");

        if (isGltf) {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.85f, 0.3f, 1.0f));
        } else if (isImage) {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.7f, 0.3f, 1.0f));
        } else if (isShader) {
          ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.3f, 0.85f, 1.0f));
        }

        bool selected = (selectedFile == entry.path());
        if (ImGui::Selectable(name.c_str(), selected,
            ImGuiSelectableFlags_SpanAllColumns)) {
          selectedFile = entry.path();
        }

        if (isGltf || isImage || isShader) {
          ImGui::PopStyleColor();
        }

        // Drag source
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
          std::string pathStr = entry.path().string();
          ImGui::SetDragDropPayload("ASSET_FILE", pathStr.c_str(), pathStr.size() + 1);
          ImGui::Text("%s", name.c_str());
          ImGui::EndDragDropSource();
        }

        // Context menu for GLTF files
        if (isGltf && ImGui::BeginPopupContextItem()) {
          if (ImGui::MenuItem("Import to Scene")) {
            importGLTF(entry.path());
          }
          if (ImGui::MenuItem("Import (Flattened)")) {
            try {
              app.getScene().loadGLTFModel(entry.path().string(), false);
              app.setStatusMessage("Imported (flat): " + name);
            } catch (const std::exception& e) {
              app.setStatusMessage(std::string("Import failed: ") + e.what());
            }
          }
          ImGui::EndPopup();
        }

        // File size column
        ImGui::TableSetColumnIndex(1);
        try {
          ImGui::TextDisabled("%s", formatFileSize(entry.file_size()).c_str());
        } catch (...) {
          ImGui::TextDisabled("?");
        }
      }

      ImGui::EndTable();
    }
  } catch (const std::filesystem::filesystem_error&) {
    ImGui::TextDisabled("Error reading directory");
  }
}

void AssetBrowserPanel::importGLTF(const std::filesystem::path& path) {
  app.importGLTFToScene(path.string());
}

void AssetBrowserPanel::handleFileDrop(const std::string& path) {
  std::filesystem::path filePath(path);
  std::string ext = filePath.extension().string();

  if (ext == ".gltf" || ext == ".glb") {
    importGLTF(filePath);
  } else {
    app.setStatusMessage("Unsupported file type: " + ext);
  }
}

} // namespace sauce::editor
