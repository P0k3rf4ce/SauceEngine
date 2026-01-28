#pragma once 

#include <app/ui/components/Text.hpp>
#include <imgui.h>
#include <string>

namespace sauce::ui {
class LabelText : public Text {
public:

    Text(const std::string& name, const std::string& label, const std::string& text);

    void render() override;

    void setLabel(const std::string& newLabel){
        label = newLabel;
    }

private:
    std::string label; 
};

}
