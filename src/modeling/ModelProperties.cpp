#include "modeling/ModelProperties.hpp"
#include "modeling/Model.hpp"
#include "modeling/ModelLoader.hpp"
#include "shared/Scene.hpp"
#include <filesystem>
#include <variant>

using namespace modeling;


// Initialize with null model and empty properties map
ModelProperties::ModelProperties(std::string gltfFilename)
    : gltfFilename(gltfFilename), model(nullptr) {

    // Check if the file path is valid and exists
    if (gltfFilename.empty()) {
        LOG_ERROR("ModelProperties: Empty filename provided");
        return;
    }

    // Check if file exists
    if (!std::filesystem::exists(gltfFilename)) {
        LOG_ERROR_F("ModelProperties: File does not exist: %s", gltfFilename.c_str());
        return;
    }

    // Create a shader for this model
    auto shader = std::make_shared<Shader>();
    if (!shader) {
        LOG_ERROR("ModelProperties: Failed to create shader");
        return;
    }

    // Load default vertex and fragment shaders
    if (!shader->loadFromFiles({
        {VERTEX, "assets/default.vert"},
        {FRAGMENT, "assets/default.frag"}
    })) {
        LOG_ERROR("ModelProperties: Failed to load shader files");
        return;
    }

    // Load the model using ModelLoader
    LOG_INFO_F("ModelProperties: Loading model from file: %s", gltfFilename.c_str());
    auto models = ModelLoader::loadModels(gltfFilename, shader);

    if (models.empty()) {
        LOG_ERROR_F("ModelProperties: Failed to load model from file: %s", gltfFilename.c_str());
        return;
    }
    
    // Use the first model (could be extended to support multiple models)
    model = models[0];
    LOG_INFO_F("ModelProperties: Successfully loaded model with %d meshes",
               static_cast<int>(model->getMeshes().size()));

    // Extract and store metadata from the model
    const auto& modelMetadata = model->getMetadata();
    for (const auto& [key, value] : modelMetadata) {
        properties[key] = value;
    }

    if (!modelMetadata.empty()) {
        LOG_INFO_F("ModelProperties: Loaded %d metadata properties from model",
                   static_cast<int>(modelMetadata.size()));

        // Log node name if available
        if (model->hasMetadata("node_name")) {
            std::string nodeName = model->getMetadataValue<std::string>("node_name");
            LOG_DEBUG_F("ModelProperties: Node name = %s", nodeName.c_str());
        }
    }

    // If there are multiple models, log a warning
    if (models.size() > 1) {
        LOG_WARN_F("ModelProperties: File contains %d models, using only the first one",
                   static_cast<int>(models.size()));
    }

    LOG_INFO("===========================================");
    LOG_INFO_F("DEBUG: Model loaded from: %s", gltfFilename.c_str());
    LOG_INFO("===========================================");

    // Print metadata/tags from properties map
    if (!properties.empty()) {
        LOG_INFO_F("Metadata/Properties (%zu):", properties.size());
        for (const auto &map_pair : properties) {
            auto key = map_pair.first;
            auto value = map_pair.second;
            std::visit([&key](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    LOG_INFO_F("  %s = \"%s\"", key.c_str(), arg.c_str());
                } else if constexpr (std::is_same_v<T, int>) {
                    LOG_INFO_F("  %s = %d", key.c_str(), arg);
                } else if constexpr (std::is_same_v<T, bool>) {
                    LOG_INFO_F("  %s = %s", key.c_str(), arg ? "true" : "false");
                } else if constexpr (std::is_same_v<T, float>) {
                    LOG_INFO_F("  %s = %.3f", key.c_str(), arg);
                } else if constexpr (std::is_same_v<T, double>) {
                    LOG_INFO_F("  %s = %.3f", key.c_str(), arg);
                }
            }, value);
        }
    } else {
        LOG_INFO("Metadata/Properties: (none)");
    }

    // Print mesh information
    const auto& meshes = model->getMeshes();
    LOG_INFO_F("Meshes: %zu", meshes.size());
    for (size_t i = 0; i < meshes.size(); i++) {
        const auto& mesh = meshes[i];
        LOG_INFO_F("  Mesh %zu:", i);
        LOG_INFO_F("    Vertices: %zu", mesh->vertices.size());
        LOG_INFO_F("    Indices: %zu (triangles: %zu)",
                   mesh->indices.size(), mesh->indices.size() / 3);

        // Print first few vertices for verification
        size_t numToPrint = std::min(mesh->vertices.size(), size_t(5));
        if (numToPrint > 0) {
            LOG_INFO("    Sample vertices:");
        }
        for (size_t v = 0; v < numToPrint; v++) {
            const auto& vert = mesh->vertices[v];
            LOG_INFO_F("      Vertex %zu: pos(%.3f, %.3f, %.3f) normal(%.3f, %.3f, %.3f) uv(%.3f, %.3f)",
                       v,
                       vert.Position.x, vert.Position.y, vert.Position.z,
                       vert.Normal.x, vert.Normal.y, vert.Normal.z,
                       vert.TexCoords.x, vert.TexCoords.y);
        }
        if (mesh->vertices.size() > 5) {
            LOG_INFO_F("      ... and %zu more vertices", mesh->vertices.size() - 5);
        }
    }

    // Print material information
    const auto& materials = model->getMaterials();
    LOG_INFO_F("Materials: %zu", materials.size());
    for (size_t i = 0; i < materials.size(); i++) {
        const auto& mat = materials[i];
        LOG_INFO_F("  Material %zu: \"%s\"", i, mat->name.c_str());

        auto printTexture = [](const char* name, const std::shared_ptr<Texture>& tex) {
            if (tex) {
                LOG_INFO_F("    %s: %ux%u, %u channels, GL ID: %u",
                           name, tex->width, tex->height, tex->n_channels, tex->id);
            } else {
                LOG_INFO_F("    %s: (null)", name);
            }
        };

        printTexture("Base Color", mat->base_color);
        printTexture("Normal", mat->normal);
        printTexture("Albedo", mat->albedo);
        printTexture("Metallic", mat->metallic);
        printTexture("Roughness", mat->roughness);
        printTexture("AO", mat->ambient_occlusion);
    }

    LOG_INFO("===========================================");
}

ModelProperties::~ModelProperties() {
    // Shared pointer will automatically clean up Model
    // Properties map will clean up automatically
}

void ModelProperties::load() {

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
            // Get camera from active scene
            auto active_scene = Scene::get_active_scene();
            if (active_scene == nullptr) {
                LOG_WARN("ModelProps update: Active scene is null, not setting view matrix. Use Scene::set_active_scene()");
            } else {
                auto cam = active_scene->get_camera();
                if (cam == nullptr) {
                    LOG_WARN("ModelProps update: Camera is null, not setting view matrix. Use Scene->set_camera()");
                } else {
                    shader->setUniform("view", cam->getView());
                }
            }
        }
    }
}

bool ModelProperties::hasProperty(const std::string& tag) const {
    return properties.find(tag) != properties.end();
}

void ModelProperties::removeProperty(const std::string& tag) {
    properties.erase(tag);
}
