#include "modeling/ModelProperties.hpp"
#include "modeling/Model.hpp"
#include "modeling/ModelLoader.hpp"
#include "shared/Logger.hpp"

using namespace modeling;

ModelProperties::ModelProperties(std::string gltfFilename) 
    : gltfFilename(gltfFilename), model(nullptr) {
    // Initialize with null model and empty properties map
}
ModelProperties::~ModelProperties() {
    // Shared pointer will automatically clean up Model
    // Properties map will clean up automatically
}

/**
 * This function is meant to load these 
 * Model properties back into use
*/
void ModelProperties::load() {
    LOG_INFO_F("ModelProperties::load() - Starting lazy load for file: %s", gltfFilename.c_str());
    
    // Check if model is already loaded
    if (model != nullptr) {
        LOG_WARN_F("ModelProperties::load() - Model already loaded for file: %s", gltfFilename.c_str());
        return;
    }
    
    // Check if filename is empty
    if (gltfFilename.empty()) {
        LOG_ERROR("ModelProperties::load() - No filename provided, cannot load model");
        return;
    }
    
    // Load models using ModelLoader
    // Note: We need a shader to load the model. For now, we'll pass nullptr
    // and expect the shader to be set later, or you may want to create a default shader
    LOG_DEBUG("ModelProperties::load() - Calling ModelLoader::loadModels()");
    std::vector<std::shared_ptr<Model>> loadedModels = ModelLoader::loadModels(gltfFilename, nullptr);
    
    // Check if any models were loaded
    if (loadedModels.empty()) {
        LOG_ERROR_F("ModelProperties::load() - Failed to load any models from file: %s", gltfFilename.c_str());
        return;
    }
    
    // Use the first model (most common case for single model files)
    model = loadedModels[0];
    LOG_INFO_F("ModelProperties::load() - Successfully loaded model with %zu meshes", 
               model->getMeshes().size());
    
    // Debug: Print detailed information about the loaded model
    auto meshes = model->getMeshes();
    auto materials = model->getMaterials();
    auto shader = model->getShader();
    
    // Log mesh information
    LOG_DEBUG_F("ModelProperties::load() - Model contains %zu meshes", meshes.size());
    for (size_t i = 0; i < meshes.size(); i++) {
        if (meshes[i]) {
            LOG_DEBUG_F("  Mesh[%zu]: %zu vertices, %zu indices", 
                       i, meshes[i]->vertices.size(), meshes[i]->indices.size());
            
        } else {
            LOG_WARN_F("  Mesh[%zu]: NULL pointer", i);
        }
    }
    
    // Log material information
    LOG_DEBUG_F("ModelProperties::load() - Model contains %zu materials", materials.size());
    for (size_t i = 0; i < materials.size(); i++) {
        if (materials[i]) {
            LOG_DEBUG_F("  Material[%zu]: Loaded successfully", i);
        } else {
            LOG_WARN_F("  Material[%zu]: NULL pointer", i);
        }
    }
    
    // Log shader information
    if (shader) {
        LOG_DEBUG("ModelProperties::load() - Shader: Loaded and attached to model");
    } else {
        LOG_WARN("ModelProperties::load() - Shader: NULL (may need to be set separately)");
    }
    
    LOG_INFO("ModelProperties::load() - Lazy loading completed successfully");
}

/**
 * This function is meant to remove these 
 * Model properties from use with the
 * intention that they will be used in the future.
*/
void ModelProperties::unload() {

}

/**
 * Do the various buffer setups to prepare the model
 * for the shader program
*/
void ModelProperties::update(const animation::AnimationProperties &animProps) {
    if (model) {
        // Setup the model for rendering (bind shader and vertex data)
        model->setupForRendering();
        
        auto shader = model->getShader();

        // Set any modeling-specific uniforms
        if (shader && shader->is_bound()) {
            
        }
    }
}

bool ModelProperties::hasProperty(const std::string& tag) const {
    return properties.find(tag) != properties.end();
}

void ModelProperties::removeProperty(const std::string& tag) {
    properties.erase(tag);
}