#pragma once

#include <app/ui/ImGuiComponent.hpp>
#include <memory>
#include <string>
#include <vector>

namespace sauce::ui {

    class ImGuiComponentManager {
      public:
        void addComponent(std::unique_ptr<ImGuiComponent> component);
        bool removeComponent(const std::string& name);
        ImGuiComponent* getComponent(const std::string& name);
        void renderAll(); // Renders all enabled components
        bool setComponentEnabled(const std::string& name, bool enabled);
        size_t getComponentCount() const;

      private:
        std::vector<std::unique_ptr<ImGuiComponent>> components;
    };

} // namespace sauce::ui
