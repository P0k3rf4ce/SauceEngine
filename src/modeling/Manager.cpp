#include "modeling/Manager.hpp"
#include "modeling/ModelLoader.hpp"
#include "stb_image.h"
#include <ranges>

void AssetManager::load_file(std::string GLTF_path) {
    for (auto &scene: this->scenes){
        if (scene.path == GLTF_path) {        // file found

            // file is still loaded
            if (!scene.was_marked_unloaded) {
                return;
            }
            scene.was_marked_unloaded = false;

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
                        // was freed, replace with loaded models
                        *maybe_model = loaded_models[i];
                    } else { 
                        // was not freed yet, inc ref count
                        *maybe_model = maybe_model->lock();
                    }
                }
            }

            return;
        }
    }
}

void AssetManager::mark_unloadable(std::string GLTF_path) {
    for (auto &scene: this->scenes) {
        if (scene.path == GLTF_path) {// file found
            scene.was_marked_unloaded = true;
            
            // for every loaded model, replace with a maybe loaded variant
            for (auto &maybe_model: scene.contents) {               
                if (auto loaded_model = std::get_if<LoadedModel>(&maybe_model)) {
                    loaded_model->reset();
                    maybe_model = MaybeUnloadedModel(*loaded_model);
                }
            }

            return;
        }
    }
}

