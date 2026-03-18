#include <algorithm>
#include <app/ui/components/Checkbox.hpp>
#include <string>

namespace sauce::ui {

    Checkbox::Checkbox(const std::string& name, const std::string& label, bool* checked,
                       Callback onChanged)
        : ImGuiComponent(name), label(label), checked(checked), onChanged(onChanged) {
    }

    void Checkbox::render() {
        if (!enabled)
            return;

        bool prev = *checked;

        if (ImGui::Checkbox(label.c_str(), checked)) {
            if (prev != *checked && onChanged) {
                onChanged(*checked);
            }
        }
    }

} // namespace sauce::ui
