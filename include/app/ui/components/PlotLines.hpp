#pragma once

#include <string>
#include <vector>
#include <app/ui/ImGuiComponent.hpp>

namespace sauce::ui
{
	class PlotLines : public ImGuiComponent
	{
	public:
		explicit PlotLines(const std::string& name, std::vector<float> values);
		~PlotLines() override;

		void render() override;

	private:
		std::vector<float> values;
	};
}
