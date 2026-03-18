#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <string>
#include <vector>

namespace sauce::ui {
    class PlotLines : public ImGuiComponent {
      public:
        explicit PlotLines(const std::string& name, std::vector<float> values);
        ~PlotLines() override;

        void render() override;

      private:
        std::vector<float> values;
    };
} // namespace sauce::ui
