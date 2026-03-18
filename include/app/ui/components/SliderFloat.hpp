#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <array>
#include <functional>
#include <imgui.h>
#include <string>

namespace sauce::ui {

    template <int N = 1> class SliderFloat : public ImGuiComponent {
        static_assert(N >= 1 && N <= 4, "SliderFloat component count must be between 1 and 4");

      public:
        using Callback = std::function<void(std::array<float, N>&)>;

        SliderFloat(const std::string& name, const std::string& label, std::array<float, N>& value,
                    float min = 0.0f, float max = 1.0f, const std::string& format = "%.3f",
                    Callback onChanged = nullptr)
            : ImGuiComponent(name), label(label), value(value), min(min), max(max), format(format),
              onChanged(onChanged) {
        }

        void render() override {
            if (!enabled)
                return;

            bool changed = false;

            if constexpr (N == 1) {
                changed = ImGui::SliderFloat(label.c_str(), value.data(), min, max, format.c_str());
            } else if constexpr (N == 2) {
                changed =
                    ImGui::SliderFloat2(label.c_str(), value.data(), min, max, format.c_str());
            } else if constexpr (N == 3) {
                changed =
                    ImGui::SliderFloat3(label.c_str(), value.data(), min, max, format.c_str());
            } else if constexpr (N == 4) {
                changed =
                    ImGui::SliderFloat4(label.c_str(), value.data(), min, max, format.c_str());
            }

            if (changed && onChanged) {
                onChanged(value);
            }
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
        float min;
        float max;
        std::string format;
        Callback onChanged;
    };

} // namespace sauce::ui
