#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <array>
#include <functional>
#include <imgui.h>
#include <string>

namespace sauce::ui {

    template <int N = 1> class SliderInt : public ImGuiComponent {
        static_assert(N >= 1 && N <= 4, "SliderInt component count must be between 1 and 4");

      public:
        using Callback = std::function<void(std::array<int, N>&)>;

        SliderInt(const std::string& name, const std::string& label, std::array<int, N>& value,
                  int min = 0, int max = 100, const std::string& format = "%d",
                  Callback onChanged = nullptr)
            : ImGuiComponent(name), label(label), value(value), min(min), max(max), format(format),
              onChanged(onChanged) {
        }

        void render() override {
            if (!enabled)
                return;

            bool changed = false;

            if constexpr (N == 1) {
                changed = ImGui::SliderInt(label.c_str(), value.data(), min, max, format.c_str());
            } else if constexpr (N == 2) {
                changed = ImGui::SliderInt2(label.c_str(), value.data(), min, max, format.c_str());
            } else if constexpr (N == 3) {
                changed = ImGui::SliderInt3(label.c_str(), value.data(), min, max, format.c_str());
            } else if constexpr (N == 4) {
                changed = ImGui::SliderInt4(label.c_str(), value.data(), min, max, format.c_str());
            }

            if (changed && onChanged) {
                onChanged(value);
            }
        }

        void setRange(int newMin, int newMax) {
            min = newMin;
            max = newMax;
        }
        void setFormat(const std::string& fmt) {
            format = fmt;
        }

      protected:
        std::string label;
        std::array<int, N>& value;
        int min;
        int max;
        std::string format;
        Callback onChanged;
    };

} // namespace sauce::ui
