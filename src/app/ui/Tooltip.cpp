#include <imgui.h>
#include <app/ui/components/Tooltip.hpp>

namespace sauce::ui
{
	Tooltip::Tooltip(const std::string& name, std::string text)
		: ImGuiComponent(name), text{std::move(text)}
	{
	}

	Tooltip::~Tooltip() = default;

	void Tooltip::render()
	{
		if (enabled && ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(this->text.c_str());
		}
	}
}
