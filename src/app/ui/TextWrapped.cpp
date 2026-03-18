#include <app/ui/components/TextWrapped.hpp>
#include <string>

namespace sauce::ui {

    TextWrapped::TextWrapped(const std::string& name, const std::string& text) : Text(name, text) {
    }

    void TextWrapped::render() {
        if (!enabled)
            return;
        ImGui::TextWrapped("%s", text.c_str());
    }

} // namespace sauce::ui
