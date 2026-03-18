#include <algorithm>
#include <app/ui/ImGuiComponentManager.hpp>

namespace sauce::ui {

    void ImGuiComponentManager::addComponent(std::unique_ptr<ImGuiComponent> component) {
        if (component) {
            components.push_back(std::move(component));
        }
    }

    bool ImGuiComponentManager::removeComponent(const std::string& name) {
        auto it = std::find_if(components.begin(), components.end(),
                               [&name](const std::unique_ptr<ImGuiComponent>& comp) {
                                   return comp->getName() == name;
                               });

        if (it != components.end()) {
            components.erase(it);
            return true;
        }
        return false;
    }

    ImGuiComponent* ImGuiComponentManager::getComponent(const std::string& name) {
        auto it = std::find_if(components.begin(), components.end(),
                               [&name](const std::unique_ptr<ImGuiComponent>& comp) {
                                   return comp->getName() == name;
                               });

        if (it != components.end()) {
            return it->get();
        }
        return nullptr;
    }

    void ImGuiComponentManager::renderAll() {
        for (const auto& component : components) {
            if (component && component->isEnabled()) {
                component->render();
            }
        }
    }

    bool ImGuiComponentManager::setComponentEnabled(const std::string& name, bool enabled) {
        ImGuiComponent* component = getComponent(name);
        if (component) {
            component->setEnabled(enabled);
            return true;
        }
        return false;
    }

    size_t ImGuiComponentManager::getComponentCount() const {
        return components.size();
    }

} // namespace sauce::ui
