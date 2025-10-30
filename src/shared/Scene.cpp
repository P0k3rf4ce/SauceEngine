#include "shared/Scene.hpp"

Scene::Scene() {
    
}

Scene::Scene(std::string &filename) {
    
}

Scene::~Scene() {

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