#pragma once 

#include <app/ui/ImGuiComponent.hpp>
#include <imgui.h>
#include <string>

namespace sauce::ui {
class Text : public ImGuiComponent {
public:

    Text(const std::string& name, const std::string& text);

    void render() override;

    void setText(const std::string& newText){
        text = newText;
    }

protected:
    std::string text; 
};

}
