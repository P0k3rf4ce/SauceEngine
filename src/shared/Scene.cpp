#include "shared/Scene.hpp"
#include "utils/Logger.hpp"

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

void Scene::set_camera(std::shared_ptr<Camera> cam) {
    if(cam == nullptr) {
        LOG_WARN("Attempted to set scene camera to nullptr");
        return;
    }
    this->active_camera = cam;

	for (auto object: this->objects) {
		object.set_camera(cam);
	}
}

std::shared_ptr<Camera> Scene::get_camera() {
    if (this->active_camera == nullptr) {
        LOG_WARN("Scene default camera is null... Creating camera with default values");
        set_camera(std::make_shared<Camera>(Vector3f(), Vector3f()));
    }
    return this->active_camera;
}
