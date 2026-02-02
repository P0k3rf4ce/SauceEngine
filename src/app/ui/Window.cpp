#include <app/ui/components/Window.hpp>

namespace sauce::ui {

Window::Window(const std::string& name, const std::string& title, ContentCallback content, 
               ImGuiWindowFlags flags)
    : ImGuiComponent(name), title(title), content(content), flags(flags) {}

void Window::render() {
    if (!enabled) return;
    
    if (ImGui::Begin(title.c_str(), &isOpen, flags)) {
        if (content) {
            content();
        }
    }
    ImGui::End();
}

}
