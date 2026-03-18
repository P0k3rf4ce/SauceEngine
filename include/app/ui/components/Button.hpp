#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <functional>
#include <imgui.h>
#include <string>

namespace sauce::ui {
    class Button : public ImGuiComponent {
      public:
        using Callback = std::function<void()>;

        Button(const std::string& name, const std::string& label, Callback onClick = nullptr);

        void render() override;

      private:
        std::string label;
        Callback onClick;
    };

} // namespace sauce::ui
