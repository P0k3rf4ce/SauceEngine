#pragma once

#include <functional>
#include <string>

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

        void load(const std::string& path = "sauceengine_settings.json");
        void save() const;
        void save(const std::string& path);

        EditorSettings& get() {
            return settings;
        }
        const EditorSettings& get() const {
            return settings;
        }

        void markDirtyAndSave();

        void setOnChangeCallback(ChangeCallback cb) {
            onChange = std::move(cb);
        }

      private:
        EditorSettings settings;
        std::string filePath = "sauceengine_settings.json";
        ChangeCallback onChange;
    };

} // namespace sauce
