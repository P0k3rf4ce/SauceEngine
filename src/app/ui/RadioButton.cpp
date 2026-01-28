#include <app/ui/components/RadioButton.hpp>
#include <algorithm>
#include <string>

namespace sauce::ui{

RadioButton::RadioButton(const std::string& name, const std::string& label, int* selected, int value, Callback onChanged)
 : ImGuiComponent(name), label(label), selected(selected), value(value), onChanged(onChanged) {} 

void RadioButton::render() { 
    if (!enabled) return; 
    int prev = *selected;
    if (ImGui::RadioButton(label.c_str() selected, value)) {
        if (*value != prev && onChanged){
            onChanged(*value);
        } 
    } 
}

}
