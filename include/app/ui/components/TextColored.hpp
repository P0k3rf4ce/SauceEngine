#pragma once

#include <app/ui/components/Text.hpp>

namespace sauce::ui {
    class TextColored : public Text {
      public:
        TextColored(const std::string& name, const std::string& text, const ImVec4& color);

        void changeColor(const ImVec4& newColor) {
            color = newColor;
        }

        void render() override;

      private:
        ImVec4 color;
    };

} // namespace sauce::ui
