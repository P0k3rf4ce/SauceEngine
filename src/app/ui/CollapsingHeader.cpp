#include <app/ui/components/CollapsingHeader.hpp>

namespace sauce::ui {

CollapsingHeader::CollapsingHeader(const std::string& name, const std::string& label, 
                                   ContentCallback content, ImGuiTreeNodeFlags flags)
    : ImGuiComponent(name), label(label), content(content), flags(flags) {}

void CollapsingHeader::render() {
    if (!enabled || !visible) return;
    
    headerOpen = ImGui::CollapsingHeader(label.c_str(), flags);
    if (headerOpen) {
        if (content) {
            content();
        }
    }
}

}
