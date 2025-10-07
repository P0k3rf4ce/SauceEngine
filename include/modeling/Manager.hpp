#pragma once
#include "modeling/Material.hpp"
#include "modeling/Model.hpp"
#include <assimp/scene.h>
#include <string>
#include <optional>

// temporary Key definition
struct ModelKey{
    int scene;
    int id;
};

// temporary Key definition
struct MeshKey{
    int scene;
    int id;
};

// temporary Key definition
struct MaterialKey{
    int scene;
    MaterialHandle id;
};

// temporary Key definition
struct TextureKey{
    int scene;
    int id;
};

// loaded GLTF file contents
struct LoadedContents {
    // all `Material` and `Textures` 
    MaterialManager materials;

    // all models and meshes
    std::vector<Model> models;

    // https://github.com/P0k3rf4ce/SauceEngine/blob/61b7983fc037b3e6f96ccd42d4905ea60c3d2fe5/src/modeling/ModelLoader.cpp
    std::vector<Mesh> &loaded_meshes;

private:
    // no copy
    LoadedContents(const LoadedContents&) = delete;
    LoadedContents& operator=(const LoadedContents&) = delete;
    
    // no move
    LoadedContents(LoadedContents&&) = delete;
    LoadedContents& operator=(LoadedContents&&) = delete;
};

// Possibly loaded GLTF file
struct SceneObjects {
    // Path to GLTF file
    std::string path;
    
    // maybe loaded contents
    std::optional<LoadedContents> contents;

private:
    // no copy
    SceneObjects(const SceneObjects&) = delete;
    SceneObjects& operator=(const SceneObjects&) = delete;
    
    // no move
    SceneObjects(SceneObjects&&) = delete;
    SceneObjects& operator=(SceneObjects&&) = delete;
};

// Manages all assets from all files
class AssetManager {
    std::vector<SceneObjects> scenes;
    std::vector<Model> custom_models;

public:
    AssetManager() = default;
    ~AssetManager() = delete;

    // loading and unloading files
    void load_file(std::string GLTF_path);
    void unload_file(std::string GLTF_path);

    // getters for assets
    const Model& get_model(ModelKey id);
    const Material& get_material(MaterialKey id);
    const Texture& get_texture(TextureKey id);
    const Mesh& get_mesh(MeshKey id);

private:
    // no copy
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    
    // no move
    AssetManager(AssetManager&&) = delete;
    AssetManager& operator=(AssetManager&&) = delete;
};