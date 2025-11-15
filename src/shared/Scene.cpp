#include "shared/Scene.hpp"

// define static active scene pointer
Scene *Scene::s_activeScene = nullptr;

Scene::Scene() {
    // set this scene as the active scene
    s_activeScene = this;
}

Scene::Scene(std::string &filename) {
    s_activeScene = this;
}

Scene::~Scene() {
    if (s_activeScene == this) {
        s_activeScene = nullptr;
    }
}

/**
 * This function is meant to load these Animation properties back into use
*/
void Scene::load() {
    for (auto object: this->objects) {
        object.load();
    }
}

/**
 * This function is meant to remove these Animation properties from use with the
 * intention that they will be used in the future.
*/
void Scene::unload() {
    for (auto object: this->objects) {
        object.unload();
    }
}

/**
 * Update the Animation properties <timestep> seconds into the future
*/
void Scene::update(double timestep) {
    for (auto object: this->objects) {
        object.update(timestep);
    }
}

const std::vector<std::shared_ptr<rendering::LightProperties>> &Scene::getLights() const noexcept {
    return lights;
}

Scene *Scene::getActiveScene() noexcept {
    return s_activeScene;
}