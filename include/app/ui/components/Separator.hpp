#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <imgui.h>
#include <string>

namespace sauce::ui {

/**
 * Separator - Wraps ImGui::Separator()
 * Creates a horizontal line separator
 */
class Separator : public ImGuiComponent {
public:
    explicit Separator(const std::string& name);

    void render() override;
};

}
