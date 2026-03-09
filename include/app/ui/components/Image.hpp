#pragma once 

#include <app/ui/ImGuiComponent.hpp>
#include <imgui.h>
#include <string>

namespace sauce::ui {
class Image : public ImGuiComponent {
public:

    Image(const std::string& name, ImTextureID texture, const ImVec2& size);

    void render() override;

    void setTexture(ImTextureID newTexture) { texture = newTexture; }
    void setSize(const ImVec2& newSize) { size = newSize; }

protected:
    ImTextureID texture;
    ImVec2 size;
};

}
