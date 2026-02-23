#include <app/ui/components/BulletText.hpp>
#include <algorithm>
#include <string>

namespace sauce::ui{

BulletText::BulletText(const std::string& name, const std::string& text)
 : Text(name, text) {} 

void BulletText::render() { 
    if (!enabled) return; 
    ImGui::BulletText("%s", text.c_str());
}

}
