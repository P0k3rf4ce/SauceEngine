#include <app/ui/components/Group.hpp>

namespace sauce::ui {

    Group::Group(const std::string& name, ContentCallback content)
        : ImGuiComponent(name), content(content) {
    }

    void Group::render() {
        if (!enabled)
            return;

        ImGui::BeginGroup();
        if (content) {
            content();
        }
        ImGui::EndGroup();
    }

} // namespace sauce::ui
