#pragma once
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include "assimp/material.h"
#include <memory>
#include <vector>
struct Texture {
    const std::unique_ptr<const uint8_t[]> data;
    const uint32_t width;
    const uint32_t height;
    const uint32_t n_channels;
    const uint32_t id;

    Texture(
        std::unique_ptr<const uint8_t[]> data,
        uint32_t width,
        uint32_t height,
        uint32_t n_channels,
        uint32_t id
    ): 
        data(std::move(data)),
        width(width),
        height(height),
        n_channels(n_channels),
        id(id) {}

    Texture() = delete;
    ~Texture() = default;
    
private:
    // no copy
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    
    // no move
    Texture(Texture&&) = delete;
    Texture& operator=(Texture&&) = delete;
};

// A Material 
struct Material {
    // name of the material
    const std::string name;

    const Texture &base_color;

    const Texture &normal;

    const Texture &albedo;

    const Texture &metallic;

    const Texture &roughness;

    // aiTextureType_AMBIENT_OCCLUSION
    const Texture &ambient_occlusion;
    
    
    Material(
        std::string name,
        Texture &base_color,
        Texture &normal,
        Texture &albedo,
        Texture &metallic,
        Texture &roughness,
        Texture &ambient_occlusion
    ):
        name(std::move(name)),
        base_color(base_color),
        normal(normal),
        albedo(albedo),
        metallic(metallic),
        roughness(roughness),
        ambient_occlusion(ambient_occlusion)
    {};
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
    explicit MaterialManager(aiScene *scene);
    ~MaterialManager() = default;

    // prevent copies
    MaterialManager(const MaterialManager&) = delete;
    MaterialManager& operator=(const MaterialManager&) = delete;
    
    // allow moves
    MaterialManager(MaterialManager&&) = default;
    MaterialManager& operator=(MaterialManager&&) = default;

    // constructor from a scene
    static MaterialManager from_aiScene(aiScene *scene);

    // returns a material from its unique handle
    const Material& get(MaterialHandle handle) const noexcept;

    // returns a reference to a material given its name
    // throws std::out_of_range the material is not found.
    const Material& find(std::string name) const;

    const Texture& get_texture(int idx);

private:
    // list of all materials in a scene 
    std::vector<Material> materials;

    // list of all textures in a scene
    std::vector<Texture> textures;
};