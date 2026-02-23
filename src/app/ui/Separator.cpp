#include <app/ui/components/Separator.hpp>

namespace sauce::ui {

Separator::Separator(const std::string& name)
    : ImGuiComponent(name) {}

void Separator::render() {
    if (!enabled) return;
    
    ImGui::Separator();
}

}
