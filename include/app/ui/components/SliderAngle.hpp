#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <functional>
#include <imgui.h>
#include <string>

namespace sauce::ui {

    class SliderAngle : public ImGuiComponent {
      public:
        using Callback = std::function<void(float)>;

        SliderAngle(const std::string& name, const std::string& label, float* valueRadians,
                    float minDegrees = -360.0f, float maxDegrees = 360.0f,
                    const std::string& format = "%.0f deg", Callback onChanged = nullptr);

        void render() override;

        void setRange(float minDegrees, float maxDegrees) {
            this->minDegrees = minDegrees;
            this->maxDegrees = maxDegrees;
        }
        void setFormat(const std::string& fmt) {
            format = fmt;
        }

      private:
        std::string label;
        float* valueRadians;
        float minDegrees;
        float maxDegrees;
        std::string format;
        Callback onChanged;
    };

} // namespace sauce::ui
