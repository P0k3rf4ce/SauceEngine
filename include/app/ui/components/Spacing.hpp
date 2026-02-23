#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <imgui.h>
#include <string>

namespace sauce::ui {

/**
 * Spacing - Wraps ImGui::Spacing()
 * Adds vertical spacing between elements
 */
class Spacing : public ImGuiComponent {
public:
    explicit Spacing(const std::string& name, int count = 1);

    void render() override;

    void setCount(int newCount) { count = newCount; }
    int getCount() const { return count; }

private:
    int count;  // Number of spacing calls to make
};

}
