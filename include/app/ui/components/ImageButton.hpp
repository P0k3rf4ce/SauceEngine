#pragma once

#include <app/ui/components/Image.hpp>
#include <functional>

namespace sauce::ui {
    class ImageButton : public Image {
      public:
        using Callback = std::function<void()>;
        ImageButton(const std::string& name, ImTextureID texture, const ImVec2& size,
                    Callback onClick = nullptr);

        void render() override;
        void setCallback(Callback cb) {
            onClick = cb;
        }

      private:
        Callback onClick;
    };

} // namespace sauce::ui
