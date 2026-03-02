#include <app/ui/components/ChildWindow.hpp>

namespace sauce::ui {

ChildWindow::ChildWindow(const std::string& name, const ImVec2& size, 
                         ImGuiChildFlags childFlags, ImGuiWindowFlags windowFlags,
                         ContentCallback content)
    : ImGuiComponent(name), size(size), childFlags(childFlags), 
      windowFlags(windowFlags), content(content) {}

void ChildWindow::render() {
    if (!enabled) return;
    
    if (ImGui::BeginChild(name.c_str(), size, childFlags, windowFlags)) {
        if (content) {
            content();
        }
    }
    ImGui::EndChild();
}

}
