#include "modeling/ModelProperties.hpp"
#include "modeling/ModelLoader.hpp"  

using namespace modeling;

static std::string s_modelPath;

ModelProperties::ModelProperties(std::string gltfFilename) {
    s_modelPath = std::move(gltfFilename);
}
ModelProperties::~ModelProperties() {

}

/**
 * This function is meant to load these 
 * Model properties back into use
*/
void ModelProperties::load() {
    ModelLoader::load(s_modelPath);
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
    
}