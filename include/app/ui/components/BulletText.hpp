#pragma once 

#include <app/ui/components/Text.hpp>

namespace sauce::ui {
class BulletText : public Text {
public:

    BulletText(const std::string& name, const std::string& text);
    
    void render() override;
};

}
