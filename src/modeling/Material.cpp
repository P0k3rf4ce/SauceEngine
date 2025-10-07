#include "modeling/Material.hpp" 
#include <iostream> 

Material Material::from_aiMaterial(aiMaterial *material) {
    throw std::runtime_error("todo for model loading");
}

MaterialManager::MaterialManager(aiScene *scene) {
    // load all textures first into this.textures
    // load materials which then reference the loaded textures
    throw std::runtime_error("todo for model loading");
}

MaterialHandle MaterialHandle::new_unchecked(size_t id) {
    return MaterialHandle{ id };
}

const Material& MaterialManager::get(MaterialHandle handle) const noexcept {
    return this->materials[handle.id];
}

const Texture& MaterialManager::get_texture(int id) {
    return this->textures[id];
}

const Material& MaterialManager::find(std::string name) const {
    for (auto it = this->materials.begin(); it != this->materials.end(); ++it) {
        if (it->name == name) {
            return *it;
        }
    }
    throw std::out_of_range(name + " is not a registered material");
}