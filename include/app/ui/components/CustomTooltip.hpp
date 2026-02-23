#pragma once

#include <functional>
#include <string>
#include <app/ui/ImGuiComponent.hpp>

namespace sauce::ui
{
	class CustomTooltip : public ImGuiComponent
	{
	public:
		explicit CustomTooltip(const std::string& name, std::function<void()> renderFn);
		~CustomTooltip() override;

		void render() override;

	private:
		std::function<void()> renderFn;
	};
}
