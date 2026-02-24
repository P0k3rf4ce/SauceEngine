#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <imgui.h>
#include <functional>
#include <string>

namespace sauce::ui {
class Checkbox : public ImGuiComponent {
public:
    using Callback = std::function<void(bool)>;

    Checkbox(const std::string& name, const std::string& label, bool* checked, Callback onChanged = nullptr);

    void render() override;

private:
    std::string label; 
    bool* checked;
    Callback onChanged;
};

}
