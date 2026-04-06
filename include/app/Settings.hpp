#pragma once

#include <string>
#include <functional>

namespace sauce {

struct EditorSettings {
  float imguiScale = 1.0f;
  bool vsync = true;
  std::string workingDirectory = ".";
  bool palantirMode = true;
  bool showDebugStats = true;
  float mouseSensitivity = 0.1f;
  float cameraSpeed = 2.5f;
  float fieldOfView = 90.0f;
};

class SettingsManager {
public:
  using ChangeCallback = std::function<void(const EditorSettings&)>;

  void load(const std::string& path = "");  // empty = use OS default
  void save() const;
  void save(const std::string& path);

  EditorSettings& get() { return settings; }
  const EditorSettings& get() const { return settings; }

  void markDirtyAndSave();

  void setOnChangeCallback(ChangeCallback cb) { onChange = std::move(cb); }

private:
  EditorSettings settings;
  std::string filePath;  // set by load() on first call
  ChangeCallback onChange;
};

} // namespace sauce
