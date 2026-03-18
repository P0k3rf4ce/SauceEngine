#include <app/Log.hpp>
#include <app/Settings.hpp>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>

namespace sauce {

    void SettingsManager::load(const std::string& path) {
        filePath = path;

        std::ifstream file(filePath);
        if (!file.is_open()) {
            SAUCE_LOG("Settings", "No config file found at {}, using defaults", filePath);
            save();
            return;
        }

        try {
            nlohmann::json j = nlohmann::json::parse(file);

            if (j.contains("imgui_scale"))
                settings.imguiScale = j["imgui_scale"].get<float>();
            if (j.contains("vsync"))
                settings.vsync = j["vsync"].get<bool>();
            if (j.contains("working_directory"))
                settings.workingDirectory = j["working_directory"].get<std::string>();
            if (j.contains("palantir_mode"))
                settings.palantirMode = j["palantir_mode"].get<bool>();
            if (j.contains("show_debug_stats"))
                settings.showDebugStats = j["show_debug_stats"].get<bool>();
            if (j.contains("mouse_sensitivity"))
                settings.mouseSensitivity = j["mouse_sensitivity"].get<float>();
            if (j.contains("camera_speed"))
                settings.cameraSpeed = j["camera_speed"].get<float>();
            if (j.contains("field_of_view"))
                settings.fieldOfView = j["field_of_view"].get<float>();

            SAUCE_LOG("Settings", "Loaded settings from {}", filePath);
        } catch (const nlohmann::json::exception& e) {
            SAUCE_LOG("Settings", "Failed to parse {}: {}, using defaults", filePath, e.what());
            settings = EditorSettings{};
        }
    }

    void SettingsManager::save() const {
        auto roundFloat = [](float v, int decimals) -> double {
            double mul = std::pow(10.0, decimals);
            return std::round(static_cast<double>(v) * mul) / mul;
        };

        nlohmann::json j = {
            {"imgui_scale", roundFloat(settings.imguiScale, 2)},
            {"vsync", settings.vsync},
            {"working_directory", settings.workingDirectory},
            {"palantir_mode", settings.palantirMode},
            {"show_debug_stats", settings.showDebugStats},
            {"mouse_sensitivity", roundFloat(settings.mouseSensitivity, 2)},
            {"camera_speed", roundFloat(settings.cameraSpeed, 1)},
            {"field_of_view", roundFloat(settings.fieldOfView, 1)},
        };

        std::ofstream file(filePath);
        if (file.is_open()) {
            file << j.dump(2) << std::endl;
            SAUCE_LOG_VERBOSE("Settings", "Settings saved to {}", filePath);
        } else {
            SAUCE_LOG("Settings", "Failed to save settings to {}", filePath);
        }
    }

    void SettingsManager::save(const std::string& path) {
        filePath = path;
        save();
    }

    void SettingsManager::markDirtyAndSave() {
        save();
        if (onChange) {
            onChange(settings);
        }
    }

} // namespace sauce
