#include "modeling/Manager.hpp"
#include "modeling/ModelLoader.hpp"
#include "stb_image.h"

void AssetManager::load_file(std::string GLTF_path) {

    // check if file has been loaded already
    for (auto &scene: this->scenes){
        if (scene.path == GLTF_path) {        // file found
            LOG_INFO_F("%s has been found", GLTF_path);
            
            // file is still loaded
            if (!scene.is_marked_unloaded) {
                LOG_INFO_F("%s is already loaded", GLTF_path);
                return;
            }
            scene.is_marked_unloaded = false;

            // load file
            auto loaded_models = modeling::ModelLoader::loadModels(GLTF_path, this->shaders);
            
            // check if changed
            if (loaded_models.size() != scene.contents.size()) {
                LOG_ERROR("Number of models have changed since this was last loaded");
                return;
            }

            // replaced possibly unloaded models with loaded ones
            for (int i = 0; i < loaded_models.size(); i++){
                if (auto maybe_model = std::get_if<MaybeUnloadedModel>(&scene.contents[i])) {
                    
                    if (maybe_model->expired()) { 
                        *maybe_model = loaded_models[i]; // was freed, replace with loaded models
                    } else { 
                        *maybe_model = maybe_model->lock(); // was not freed yet, inc ref count
                    }
                }
            }
            LOG_INFO_F("%s reloaded into memory", GLTF_path);
            return;
        }
    }

    // file has not been loaded yet.
    LOG_INFO_F("Found new file: %s", GLTF_path);

    auto loaded_models = modeling::ModelLoader::loadModels(GLTF_path, this->shaders);
    MaybeContents contents(loaded_models.size());
    for (int i = 0; i < loaded_models.size(); i++){
        contents.push_back(loaded_models[i]);
    }
    
    this->scenes.push_back(SceneObjects {
        GLTF_path,
        false,
        contents
    });
    LOG_INFO_F("%s added to manager", GLTF_path);

}

void AssetManager::mark_unloadable(std::string GLTF_path) {
    for (auto &scene: this->scenes) {
        if (scene.path == GLTF_path) {// file found
            LOG_INFO_F("%s found", GLTF_path);
            scene.is_marked_unloaded = true;
            
            // for every loaded model, replace with a maybe loaded variant
            for (auto &maybe_model: scene.contents) {               
                if (auto loaded_model = std::get_if<LoadedModel>(&maybe_model)) {
                    loaded_model->reset();
                    maybe_model = MaybeUnloadedModel(*loaded_model);
                }
            }
            LOG_INFO_F("%s marked unloadable", GLTF_path);
            return;
        }
    }

    LOG_ERROR_F("Attempted to unload unregistered file: %s", GLTF_path);
}

