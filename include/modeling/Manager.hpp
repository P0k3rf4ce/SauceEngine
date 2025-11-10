#pragma once
#include "modeling/Material.hpp"
#include "modeling/Model.hpp"
#include "modeling/Mesh.hpp"
#include <assimp/scene.h>
#include <string>
#include <optional>
#include <variant>

using LoadedModel = std::shared_ptr<modeling::Model>;
using MaybeUnloadedModel = std::weak_ptr<modeling::Model>;
using MaybeContents = std::vector<std::variant<LoadedModel, MaybeUnloadedModel>>;


// Possibly loaded GLTF file
struct SceneObjects {
    // Path to GLTF file
    std::string path;

    // has this file been marked for garbage collection
    bool is_marked_unloaded;

    // maybe loaded contents
    MaybeContents contents;

    // explicit default constructor
    SceneObjects(const std::string &path, bool is_marked_unloaded, const MaybeContents &contents) :
    path(path), is_marked_unloaded(is_marked_unloaded), contents(contents) {}

    // no copy
    SceneObjects(const SceneObjects&) = delete;
    SceneObjects& operator=(const SceneObjects&) = delete;
    
    // allow move
    SceneObjects(SceneObjects&&) = default;
    SceneObjects& operator=(SceneObjects&&) = default;
};

// Manages all assets from all files
class AssetManager {
    
    // all possibly loaded files 
    std::vector<SceneObjects> scenes;

    // unused store for custom models
    std::vector<modeling::Model> custom_models;

    // idk
    std::shared_ptr<Shader> shaders;

public:
    AssetManager() = default;
    ~AssetManager() = default;

    // loading and unloading files
    void load_file(std::string GLTF_path);
    void mark_unloadable(std::string GLTF_path);

    // used for testing
    std::vector<SceneObjects>& get_scenes() { return this->scenes; }

private:
    // no copy
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;
    
    // no move
    AssetManager(AssetManager&&) = delete;
    AssetManager& operator=(AssetManager&&) = delete;
};