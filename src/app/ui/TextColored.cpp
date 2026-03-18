#include <app/ui/components/TextColored.hpp>
#include <string>

namespace sauce::ui {

    TextColored::TextColored(const std::string& name, const std::string& text, const ImVec4& color)
        : Text(name, text), color(color) {
    }

    void TextColored::render() {
        if (!enabled)
            return;
        ImGui::TextColored(color, "%s", text.c_str());
    }

} // namespace sauce::ui
