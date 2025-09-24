#include "material.hpp" 
#include <iostream> 

Material Material::from_aiMaterial(aiMaterial *material) {
    std::string name = std::string(material->GetName().C_Str());
    
    float roughness;
    if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) != aiReturn::aiReturn_SUCCESS) {
        std::cerr << "failed to get roughness" << std::endl;
    }

    float metallic;
    if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) != aiReturn::aiReturn_SUCCESS) {
        std::cerr << "failed to get metallic" << std::endl;
    }

    float opacity;
    if (material->Get(AI_MATKEY_OPACITY, opacity) != aiReturn::aiReturn_SUCCESS) {
        std::cerr << "failed to get opacity" << std::endl;
    }

    aiColor4D ambient_tmp;
    if (material->Get(AI_MATKEY_COLOR_AMBIENT, ambient_tmp) != aiReturn::aiReturn_SUCCESS) {
        std::cerr << "failed to get ambient" << std::endl;
    }
    glm::vec4 ambient(ambient_tmp.r, ambient_tmp.g, ambient_tmp.b, ambient_tmp.a);
    
    aiColor4D diffuse_tmp;
    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse_tmp) != aiReturn::aiReturn_SUCCESS) {
        std::cerr << "failed to get diffuse" << std::endl;
    }
    glm::vec4 diffuse(diffuse_tmp.r, diffuse_tmp.g, diffuse_tmp.b, diffuse_tmp.a);
    

    aiColor4D specular_tmp;
    if (material->Get(AI_MATKEY_COLOR_SPECULAR, specular_tmp) != aiReturn::aiReturn_SUCCESS) {
        std::cerr << "failed to get specular" << std::endl;
    }
    glm::vec4 specular(specular_tmp.r, specular_tmp.g, specular_tmp.b, specular_tmp.a);

    
    aiColor4D emissive_tmp;
    if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive_tmp) != aiReturn::aiReturn_SUCCESS) {
        std::cerr << "failed to get emissive" << std::endl;
    }
    glm::vec4 emissive(emissive_tmp.r, emissive_tmp.g, emissive_tmp.b, emissive_tmp.a);
    
    return Material {
        name,      
        roughness, 
        metallic,  
        opacity,   
        ambient,   
        diffuse,   
        specular,  
        emissive   
    };
}

MaterialManager MaterialManager::from_aiScene(aiScene scene) {
    std::unordered_set<std::string> seen;

    MaterialManager manager;
    manager.materials.reserve(scene.mNumMaterials);
    for (size_t i = 0; i < scene.mNumMaterials; i++) {
        auto material = Material::from_aiMaterial(scene.mMaterials[i]);
        if (seen.find(material.name) != seen.end()) {
            std::cerr << "duplicate name found: " << material.name << " (ignoring)" << std::endl;
        } else {
            seen.insert(material.name);
        }
        manager.materials.emplace_back(material);
    }
    return manager;
}

MaterialHandle MaterialHandle::new_unchecked(size_t id) {
    return MaterialHandle{ id };
}

const Material& MaterialManager::get(MaterialHandle handle) const noexcept {
    this->materials[handle.id];
}

const Material& MaterialManager::find(std::string name) const {
    for (auto it = this->materials.begin(); it != this->materials.end(); ++it) {
        if (it->name == name) {
            return *it;
        }
    }
    throw std::out_of_range(name + " is not a registered material");
}



