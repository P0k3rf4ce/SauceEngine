#include <app/ui/components/Columns.hpp>

namespace sauce::ui {

Columns::Columns(const std::string& name, int count, const std::string& id, 
                 bool border, ContentCallback content)
    : ImGuiComponent(name), count(count), id(id), border(border), content(content) {}

void Columns::render() {
    if (!enabled) return;
    
    // Use the legacy Columns API for simplicity
    ImGui::Columns(count, id.empty() ? nullptr : id.c_str(), border);
    
    if (content) {
        content();
    }
    
    // Reset to single column
    ImGui::Columns(1);
}

}
