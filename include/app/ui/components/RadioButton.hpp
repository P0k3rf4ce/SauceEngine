#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <functional>
#include <imgui.h>
#include <string>

namespace sauce::ui {
    class RadioButton : public ImGuiComponent {
      public:
        using Callback = std::function<void(int)>;

        RadioButton(const std::string& name, const std::string& label, int* selected, int value,
                    Callback onChanged = nullptr);

        void render() override;

      private:
        std::string label;
        int* selected;
        int value;
        Callback onChanged;
    };

} // namespace sauce::ui
