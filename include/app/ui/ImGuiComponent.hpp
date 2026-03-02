#pragma once

#include <string>

namespace sauce::ui {

class ImGuiComponent {
public:
    explicit ImGuiComponent(const std::string& componentName, bool startEnabled = true)
        : name(componentName), enabled(startEnabled) {}

    virtual ~ImGuiComponent() = default;

    virtual void render() = 0;  // Override to define UI

    void setEnabled(bool state) { enabled = state; }
    bool isEnabled() const { return enabled; }
    const std::string& getName() const { return name; }

protected:
    std::string name;
    bool enabled;
};

}
