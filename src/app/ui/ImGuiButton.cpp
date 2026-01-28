#include <app/ui/components/ImGuiButton.hpp>
#include <algorithm>
#include <string>

namespace sauce::ui{

ButtonComponent::ButtonComponent(const std::string& name, const std::string& label, Callback onClick)
 : ImGuiComponent(name), label(label), onClick(onClick) {} 

void ButtonComponent::render() { 
    if (!enabled) return; 
    if (ImGui::Button(label.c_str())) {
        if (onClick){
            onClick();
        } 
    } 

}


}


