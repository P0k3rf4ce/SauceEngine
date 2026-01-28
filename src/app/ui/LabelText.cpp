#include <app/ui/components/LabelText.hpp>
#include <algorithm>
#include <string>

namespace sauce::ui{

LabelText::LabelText(const std::string& name, const std::string label, const std::string& text)
 : Text(name, text), label(label) {} 

void Text::render() { 
    if (!enabled) return;
    ImGui::LabelText(label.c_str(), "%s", text.c_str()); 
}

}
