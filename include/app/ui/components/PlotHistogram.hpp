#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <string>
#include <vector>

namespace sauce::ui {
    class PlotHistogram : public ImGuiComponent {
      public:
        explicit PlotHistogram(const std::string& name, std::vector<float> values);
        ~PlotHistogram() override;

        void render() override;

      private:
        std::vector<float> values;
    };
} // namespace sauce::ui
