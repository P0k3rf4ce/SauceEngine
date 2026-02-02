#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <imgui.h>
#include <functional>
#include <string>
#include <array>

namespace sauce::ui {

template<int N = 1>
class DragInt : public ImGuiComponent {
    static_assert(N >= 1 && N <= 4, "DragInt component count must be between 1 and 4");

public:
    using Callback = std::function<void(std::array<int, N>&)>;

    DragInt(const std::string& name, const std::string& label, std::array<int, N>& value,
            float speed = 1.0f, int min = 0, int max = 0,
            const std::string& format = "%d", Callback onChanged = nullptr)
        : ImGuiComponent(name), label(label), value(value), speed(speed), min(min), max(max),
          format(format), onChanged(onChanged) {}

    void render() override {
        if (!enabled) return;

        bool changed = false;

        if constexpr (N == 1) {
            changed = ImGui::DragInt(label.c_str(), value.data(), speed, min, max, format.c_str());
        } else if constexpr (N == 2) {
            changed = ImGui::DragInt2(label.c_str(), value.data(), speed, min, max, format.c_str());
        } else if constexpr (N == 3) {
            changed = ImGui::DragInt3(label.c_str(), value.data(), speed, min, max, format.c_str());
        } else if constexpr (N == 4) {
            changed = ImGui::DragInt4(label.c_str(), value.data(), speed, min, max, format.c_str());
        }

        if (changed && onChanged) {
            onChanged(value);
        }
    }

    void setSpeed(float spd) { speed = spd; }
    void setRange(int newMin, int newMax) { min = newMin; max = newMax; }
    void setFormat(const std::string& fmt) { format = fmt; }

protected:
    std::string label;
    std::array<int, N>& value;
    float speed;
    int min;
    int max;
    std::string format;
    Callback onChanged;
};

}
