#include <algorithm>
#include <app/ui/components/RadioButton.hpp>
#include <string>

namespace sauce::ui {

    RadioButton::RadioButton(const std::string& name, const std::string& label, int* selected,
                             int value, Callback onChanged)
        : ImGuiComponent(name), label(label), selected(selected), value(value),
          onChanged(onChanged) {
    }

    void RadioButton::render() {
        if (!enabled)
            return;
        if (ImGui::RadioButton(label.c_str(), selected, value)) {
            if (onChanged) {
                onChanged(value);
            }
        }
    }

} // namespace sauce::ui
