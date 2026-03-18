#include <algorithm>
#include <app/ui/components/Image.hpp>
#include <string>

namespace sauce::ui {

    Image::Image(const std::string& name, ImTextureID texture, const ImVec2& size)
        : ImGuiComponent(name), texture(texture), size(size) {
    }

    void Image::render() {
        if (!enabled)
            return;
        ImGui::Image(texture, size);
    }

} // namespace sauce::ui
