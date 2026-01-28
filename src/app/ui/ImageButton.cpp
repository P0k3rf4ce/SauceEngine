#include <app/ui/components/Text.hpp>
#include <algorithm>
#include <string>

namespace sauce::ui{

ImageButton::ImageButton(const std::string& name, ImTextureID texture, const ImVec2& size, Callback onClick)
 : Image(name, texture, size), onClick(onClick) {} 

void ImageButton::render() { 
    if (!enabled) return;
    if (ImGui::ImageButton(texture, size)){
        if (onClick){
            onClick();
        }
    }
}

}
