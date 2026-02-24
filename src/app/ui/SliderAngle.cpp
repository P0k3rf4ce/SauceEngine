#include <app/ui/components/SliderAngle.hpp>
#include <string>

namespace sauce::ui {

SliderAngle::SliderAngle(const std::string& name, const std::string& label, float* valueRadians,
                         float minDegrees, float maxDegrees,
                         const std::string& format, Callback onChanged)
    : ImGuiComponent(name), label(label), valueRadians(valueRadians),
      minDegrees(minDegrees), maxDegrees(maxDegrees), format(format), onChanged(onChanged) {}

void SliderAngle::render() {
    if (!enabled) return;

    if (ImGui::SliderAngle(label.c_str(), valueRadians, minDegrees, maxDegrees, format.c_str())) {
        if (onChanged) {
            onChanged(*valueRadians);
        }
    }
}

}
