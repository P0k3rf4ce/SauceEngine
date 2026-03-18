#include <algorithm>
#include <app/ui/components/Text.hpp>
#include <string>

namespace sauce::ui {

    Text::Text(const std::string& name, const std::string& text)
        : ImGuiComponent(name), text(text) {
    }

    void Text::render() {
        if (!enabled)
            return;
        ImGui::Text("%s", text.c_str());
    }

} // namespace sauce::ui
