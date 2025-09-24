#pragma once
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include "assimp/material.h"

// A Material 
struct Material {
    // name of the material
    const std::string name;

    // how reflective the material is
    // 0.0 = smooth 
    // 1.0 = rough
    const float roughness;      

    // how metallic the surface is
    // 0.0 = dielectric
    // 1.0 = metal
    const float metallic;       

    // how transparent the material is
    // 0.0 = opaque
    // 1.0 = transparent
    const float opacity;        
    
    // Uniform general illumination
    const glm::vec4 ambient;    
    
    // Color and brightness from light source
    const glm::vec4 diffuse;    

    // Shiny bight lights from reflections
    const glm::vec4 specular;   

    // Emitting color and light
    const glm::vec4 emissive;   
    
    Material(
        std::string name,
        float roughness,
        float metallic,
        float opacity,
        glm::vec4 ambient,
        glm::vec4 diffuse,
        glm::vec4 specular,
        glm::vec4 emissive
    ):
        name(std::move(name)),
        roughness(roughness),
        metallic(metallic),
        opacity(opacity),
        ambient(ambient),
        emissive(emissive),
        diffuse(diffuse),
        specular(specular)
    {
        // validation
        assert(0.0 <= roughness && roughness <= 1.0 && "roughness must be within [0.0, 1.0]");
        assert(0.0 <= metallic  && metallic  <= 1.0 && "metallic must be within [0.0, 1.0]");
        assert(0.0 <= opacity   && opacity   <= 1.0 && "opacity must be within [0.0, 1.0]");
    };
    ~Material() = default;

    // constructs a `Material` from an imported aiMaterial
    static Material from_aiMaterial(aiMaterial *material);

private:
    // prevent copies
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    // prevent moving
    Material(Material&&) = delete;
    Material& operator=(Material&&) = delete;

};

// A unique index into `MaterialManager` 
// to retrieve a `Material` 
struct MaterialHandle {
public:
    friend class MaterialManager;

    // creates a `MaterialHandle` without checking if its valid
    static MaterialHandle new_unchecked(size_t id);

private:
    size_t id;
    MaterialHandle(size_t id): id(id) {}
};

// A Context managing all imported `Material`s
// that exist within a scene
class MaterialManager {
public:

    // constructor from an imported scene
    MaterialManager() = default;
    ~MaterialManager() = default;

    // prevent copies
    MaterialManager(const MaterialManager&) = delete;
    MaterialManager& operator=(const MaterialManager&) = delete;
    
    // allow moves
    MaterialManager(MaterialManager&&) = default;
    MaterialManager& operator=(MaterialManager&&) = default;

    // constructor from a scene
    static MaterialManager from_aiScene(aiScene scene);

    // returns a material from its unique handle
    const Material& get(MaterialHandle handle) const noexcept;

    // returns a reference to a material given its name
    // throws std::out_of_range the material is not found.
    const Material& find(std::string name) const;


private:
    // list of all materials in a scene 
    std::vector<Material> materials;
};