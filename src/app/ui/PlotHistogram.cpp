#include <imgui.h>
#include <app/ui/components/PlotHistogram.hpp>

namespace sauce::ui
{
	PlotHistogram::PlotHistogram(const std::string& name, std::vector<float> values)
		: ImGuiComponent(name), values{std::move(values)}
	{
	}

	PlotHistogram::~PlotHistogram() = default;

	void PlotHistogram::render()
	{
		if (enabled)
		{
			ImGui::PlotHistogram(this->name.c_str(), this->values.data(), this->values.size());
		}
	}
}
