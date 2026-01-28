#pragma once 

#include <app/ui/ImGuiComponent.hpp>
#include <imgui.h>
#include <functional>
#include <string>

namespace sauce::ui {
class RadioButton : public ImGuiComponent {
public:
    using Callback = std::function<void()>;

    RadioButton(const std::string& name, const std::string& label, int* selected, int value, Callback onChanged = nullptr);

    void render() override;

private:
    std::string label; 
    int* selected;
    int value;
    Callback onChanged;
};

}

