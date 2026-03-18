#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <imgui.h>

namespace sauce::ui {

    class DebugStatsWindow : public ImGuiComponent {
      public:
        DebugStatsWindow() : ImGuiComponent("DebugStatsWindow") {
        }

        void render() override {
            ImGui::Begin("SauceEngine Debug");
            ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
            ImGui::Text("Frame Time: %.3f ms", 1000.0f / ImGui::GetIO().Framerate);
            ImGui::End();
        }
    };

} // namespace sauce::ui
