#pragma once

#include <string>
#include <app/ui/ImGuiComponent.hpp>

namespace sauce::ui
{
	class ProgressBar : public ImGuiComponent
	{
	public:
		explicit ProgressBar(const std::string& name, float fraction);
		~ProgressBar() override;

		void render() override;

	private:
		float fraction;
	};
}
