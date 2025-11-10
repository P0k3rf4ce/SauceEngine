#pragma once
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include "assimp/material.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

struct Texture {
    std::unique_ptr<const uint8_t[]> data;
    uint32_t width;
    uint32_t height;
    uint32_t n_channels;
    uint32_t id;

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

    // Allow move operations (needed for vectors and shared_ptr)
    Texture(Texture&& other) noexcept = default;
    Texture& operator=(Texture&& other) noexcept = default;

private:
    // Prevent copying (unique ownership of data)
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
};

// A Material
struct Material {
    // name of the material
    std::string name;

    std::shared_ptr<Texture> base_color;
    std::shared_ptr<Texture> normal;
    std::shared_ptr<Texture> albedo;
    std::shared_ptr<Texture> metallic;
    std::shared_ptr<Texture> roughness;
    std::shared_ptr<Texture> ambient_occlusion;

    Material(
        std::string name,
        std::shared_ptr<Texture> base_color,
        std::shared_ptr<Texture> normal,
        std::shared_ptr<Texture> albedo,
        std::shared_ptr<Texture> metallic,
        std::shared_ptr<Texture> roughness,
        std::shared_ptr<Texture> ambient_occlusion
    ):
        name(std::move(name)),
        base_color(std::move(base_color)),
        normal(std::move(normal)),
        albedo(std::move(albedo)),
        metallic(std::move(metallic)),
        roughness(std::move(roughness)),
        ambient_occlusion(std::move(ambient_occlusion))
    {}

    ~Material() = default;

    // Allow copying (shared_ptr can be copied)
    Material(const Material&) = default;
    Material& operator=(const Material&) = default;

    // Allow moving
    Material(Material&&) noexcept = default;
    Material& operator=(Material&&) noexcept = default;

    // constructs a `Material` from an imported aiMaterial
    static Material from_aiMaterial(aiMaterial *material);
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

// Texture cache for sharing textures across materials
class TextureCache {
public:
    // Get or load a texture from file path
    std::shared_ptr<Texture> getTexture(const std::string& path);

    // Get or load an embedded texture
    std::shared_ptr<Texture> getEmbeddedTexture(const aiTexture* aiTex);

    // Get the default white texture
    std::shared_ptr<Texture> getDefaultTexture();

    // Clear the cache
    void clear();

private:
    std::unordered_map<std::string, std::shared_ptr<Texture>> textureCache;
    std::shared_ptr<Texture> defaultTexture;
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
    const std::shared_ptr<Material> get(MaterialHandle handle) const noexcept;

    // returns a shared_ptr to a material given its name
    // throws std::out_of_range the material is not found.
    std::shared_ptr<Material> find(std::string name) const;

    std::shared_ptr<Texture> get_texture(int idx);

private:
    // list of all materials in a scene
    std::vector<std::shared_ptr<Material>> materials;

    // Texture cache for this scene
    TextureCache textureCache;
};