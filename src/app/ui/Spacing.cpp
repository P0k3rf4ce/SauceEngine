#include <app/ui/components/Spacing.hpp>

namespace sauce::ui {

    Spacing::Spacing(const std::string& name, int count) : ImGuiComponent(name), count(count) {
    }

    void Spacing::render() {
        if (!enabled)
            return;

        for (int i = 0; i < count; ++i) {
            ImGui::Spacing();
        }
    }

} // namespace sauce::ui
