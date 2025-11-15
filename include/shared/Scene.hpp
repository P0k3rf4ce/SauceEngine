#ifndef SCENE_HPP
#define SCENE_HPP

#include <string>
#include <vector>
#include <memory>

#include "shared/Object.hpp"
#include "rendering/LightProperties.hpp"

class Scene {
private:
    std::vector<Object> objects;

    // store lights as polymorphic objects (shared ownership)
    std::vector<std::shared_ptr<rendering::LightProperties>> lights;

    // pointer to the currently active scene
    static Scene *s_activeScene;
public:
    Scene();
    Scene(std::string &filename);
    ~Scene();

    // getters
    const std::vector<std::shared_ptr<rendering::LightProperties>> &getLights() const noexcept;
    static Scene *getActiveScene() noexcept;

    void load();
    void unload();
    void update(double timestep);
};

#endif