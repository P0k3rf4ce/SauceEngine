#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <imgui.h>

namespace sauce::ui {

    class HelloWorldWindow : public ImGuiComponent {
      public:
        HelloWorldWindow() : ImGuiComponent("HelloWorldWindow") {
        }

        void render() override {
            ImGui::Begin("Hello World");
            ImGui::Text("Hello World from SauceEngine UI System!");
            ImGui::Separator();
            ImGui::Text("This is an extensible ImGui component.");
            ImGui::Text("Create your own by inheriting from ImGuiComponent!");
            ImGui::End();
        }
    };

} // namespace sauce::ui
