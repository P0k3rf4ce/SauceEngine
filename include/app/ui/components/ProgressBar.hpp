#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <string>

namespace sauce::ui {
    class ProgressBar : public ImGuiComponent {
      public:
        explicit ProgressBar(const std::string& name, float fraction);
        ~ProgressBar() override;

        void render() override;

      private:
        float fraction;
    };
} // namespace sauce::ui
