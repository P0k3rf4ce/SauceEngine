#include "modeling/Manager.hpp"
#include "stb_image.h"



void AssetManager::load_file(std::string GLTF_path) {
    for (auto &scene: this->scenes){
        if (scene.path == GLTF_path) {        // file found
            if (!scene.contents.has_value()){ // value was unloaded
                // load scene
                throw std::runtime_error("TODO: depends on loading from GLTF");
            }
            return;
        }
    }
}

void AssetManager::unload_file(std::string GLTF_path) {
    for (auto &scene: this->scenes){
        if (scene.path == GLTF_path) {       // file found
            if (scene.contents.has_value()){ // scene is loaded
                scene.contents.reset();      // unload 
            }
            return;
        }
    }
}

const Model& AssetManager::get_model(ModelKey key) {
    auto& contents = this->scenes[key.scene].contents;
    if (contents.has_value()) {
        return contents->models[key.id];
    }
    throw std::runtime_error("should not happen");
}

const Material& AssetManager::get_material(MaterialKey key) {
    auto& contents = this->scenes[key.scene].contents;
    if (contents.has_value()) {
        return contents->materials.get(key.id);
    }

    throw std::runtime_error("should not happen");
}

const Texture& AssetManager::get_texture(TextureKey key) {
    auto& contents = this->scenes[key.scene].contents;
    if (contents.has_value()) {
        return contents->materials.get_texture(key.id);
    }

    throw std::runtime_error("should not happen");
}

const Mesh& AssetManager::get_mesh(MeshKey key) {
    auto& contents = this->scenes[key.scene].contents;
    if (contents.has_value()) {
        return contents->loaded_meshes[key.id];
    }

    throw std::runtime_error("should not happen");
}

