#pragma once 

#include <app/ui/components/Text.hpp>

namespace sauce::ui {
class TextWrapped : public Text {
public:

    TextWarapped(const std::string& name, const std::string& text);

    void render() override;
};

}
