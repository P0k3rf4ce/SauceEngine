#pragma once

#include <string>
#include <app/ui/ImGuiComponent.hpp>

namespace sauce::ui
{
	class Tooltip : public ImGuiComponent
	{
	public:
		explicit Tooltip(const std::string& name, std::string text);
		~Tooltip() override;

		void render() override;

	private:
		std::string text;
	};
}
