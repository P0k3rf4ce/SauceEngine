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
double Scene::update(double deltatime, double DELTA_STEP) {
    while (deltatime >= DELTA_STEP) {
        for (auto object: this->objects) {
            object.updateAnimation(DELTA_STEP);
        }
        deltatime -= DELTA_STEP;
    }

    for (auto object: this->objects) {
        object.updateModeling();
    }

    for (auto object: this->objects) {
        object.updateRendering();
    }

    return deltatime;
}

const std::vector<std::shared_ptr<rendering::LightProperties>> &Scene::getLights() const noexcept {
    return lights;
}

Scene *Scene::getActiveScene() noexcept {
    return s_activeScene;
}

// self-note to emmy here
void Scene::draw(rendering::Shader& shader) {
    for (auto object: this->objects) {
        object.draw(shader);
    }
}
