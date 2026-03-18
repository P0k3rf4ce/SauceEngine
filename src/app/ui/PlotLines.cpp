#include <app/ui/components/PlotLines.hpp>
#include <imgui.h>

namespace sauce::ui {
    PlotLines::PlotLines(const std::string& name, std::vector<float> values)
        : ImGuiComponent(name), values{std::move(values)} {
    }

    PlotLines::~PlotLines() = default;

    void PlotLines::render() {
        if (enabled) {
            ImGui::PlotLines(this->name.c_str(), this->values.data(), this->values.size());
        }
    }
} // namespace sauce::ui
