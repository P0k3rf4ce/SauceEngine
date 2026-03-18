#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <array>
#include <functional>
#include <imgui.h>
#include <string>

namespace sauce::ui {

    template <int N = 1> class DragFloat : public ImGuiComponent {
        static_assert(N >= 1 && N <= 4, "DragFloat component count must be between 1 and 4");

      public:
        using Callback = std::function<void(std::array<float, N>&)>;

        DragFloat(const std::string& name, const std::string& label, std::array<float, N>& value,
                  float speed = 1.0f, float min = 0.0f, float max = 0.0f,
                  const std::string& format = "%.3f", Callback onChanged = nullptr)
            : ImGuiComponent(name), label(label), value(value), speed(speed), min(min), max(max),
              format(format), onChanged(onChanged) {
        }

        void render() override {
            if (!enabled)
                return;

            bool changed = false;

            if constexpr (N == 1) {
                changed =
                    ImGui::DragFloat(label.c_str(), value.data(), speed, min, max, format.c_str());
            } else if constexpr (N == 2) {
                changed =
                    ImGui::DragFloat2(label.c_str(), value.data(), speed, min, max, format.c_str());
            } else if constexpr (N == 3) {
                changed =
                    ImGui::DragFloat3(label.c_str(), value.data(), speed, min, max, format.c_str());
            } else if constexpr (N == 4) {
                changed =
                    ImGui::DragFloat4(label.c_str(), value.data(), speed, min, max, format.c_str());
            }

            if (changed && onChanged) {
                onChanged(value);
            }
        }

        void setSpeed(float spd) {
            speed = spd;
        }
        void setRange(float newMin, float newMax) {
            min = newMin;
            max = newMax;
        }
        void setFormat(const std::string& fmt) {
            format = fmt;
        }

      protected:
        std::string label;
        std::array<float, N>& value;
        float speed;
        float min;
        float max;
        std::string format;
        Callback onChanged;
    };

} // namespace sauce::ui
