#include <algorithm>
#include <app/ui/components/Button.hpp>
#include <string>

namespace sauce::ui {

    Button::Button(const std::string& name, const std::string& label, Callback onClick)
        : ImGuiComponent(name), label(label), onClick(onClick) {
    }

    void Button::render() {
        if (!enabled)
            return;
        if (ImGui::Button(label.c_str())) {
            if (onClick) {
                onClick();
            }
        }
    }

} // namespace sauce::ui
