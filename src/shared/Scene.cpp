#include "shared/Scene.hpp"
#include "utils/Logger.hpp"
#include "modeling/ModelLoader.hpp"
#include <filesystem>

// Initialize static member
std::shared_ptr<Scene> Scene::active_scene = nullptr;

Scene::Scene() {
    active_camera = std::make_shared<Camera>(Vector3f(0, 0, 0), Vector3f(0, 0, 1));
}

Scene::Scene(std::string &filename) {
    active_camera = std::make_shared<Camera>(Vector3f(0, 0, 0), Vector3f(0, 0, 1));

    // Load the GLTF scene file
    if (filename.empty()) {
        LOG_ERROR("Scene: Cannot load scene from empty filename");
        return;
    }

    if (!std::filesystem::exists(filename)) {
        LOG_ERROR_F("Scene: File does not exist: %s", filename.c_str());
        return;
    }

    LOG_INFO_F("Scene: Loading scene from file: %s", filename.c_str());

    // Use ModelLoader to parse the GLTF file
    // We'll create a shader for the scene
    auto shader = std::make_shared<Shader>();
    if (!shader) {
        LOG_ERROR("Scene: Failed to create shader");
        return;
    }

    // Load all models from the GLTF file
    auto models = modeling::ModelLoader::loadModels(filename, shader);

    if (models.empty()) {
        LOG_WARN_F("Scene: No models loaded from file: %s", filename.c_str());
        return;
    }

    LOG_INFO_F("Scene: Loaded %d models from file", static_cast<int>(models.size()));

    // Group models by node name to create Objects
    // Each unique node becomes one Object
    std::unordered_map<std::string, std::vector<std::shared_ptr<modeling::Model>>> nodeGroups;

    for (const auto& model : models) {
        std::string nodeName = "default";

        // Get node name from model metadata
        if (model->hasMetadata("node_name")) {
            nodeName = model->getMetadataValue<std::string>("node_name");
        }

        nodeGroups[nodeName].push_back(model);
    }

    LOG_INFO_F("Scene: Grouped models into %d nodes", static_cast<int>(nodeGroups.size()));

    // Create one Object for the entire scene
    // The Object constructor will internally use ModelLoader and get the first model
    // TODO: Future enhancement - support creating multiple Objects from multi-object GLTF files
    // This would require extending Object to accept a Model directly, or loading each model separately
    try {
        Object obj(filename);
        objects.push_back(std::move(obj));
        LOG_INFO_F("Scene: Created Object from GLTF file with %d models", static_cast<int>(models.size()));
    } catch (const std::exception& e) {
        LOG_ERROR_F("Scene: Failed to create Object: %s", e.what());
    }
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

void Scene::set_camera(std::shared_ptr<Camera> cam) {
    if(cam == nullptr) {
        LOG_WARN("Attempted to set scene camera to nullptr");
        return;
    }
    this->active_camera = cam;
}

std::shared_ptr<Camera> Scene::get_camera() {
    if (this->active_camera == nullptr) {
        LOG_WARN("Scene default camera is null... Creating camera with default values");
        set_camera(std::make_shared<Camera>(Vector3f(), Vector3f()));
    }
    return this->active_camera;
}

std::shared_ptr<Scene> Scene::get_active_scene() {
    return active_scene;
}

void Scene::set_active_scene(std::shared_ptr<Scene> s) {
    if(s == nullptr) {
        LOG_WARN("Attempted to set active scene to nullptr");
        return;
    }
    active_scene = s;
}
