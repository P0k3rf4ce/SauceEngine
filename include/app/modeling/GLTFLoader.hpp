#pragma once

#include "app/modeling/Model.hpp"
#include "app/modeling/TextureCache.hpp"
#include <string>
#include <vector>
#include <memory>

// Forward declaration to avoid including tinygltf in header
namespace tinygltf {
    class Model;
    class Node;
    class Mesh;
    class Primitive;
    class Material;
    class Texture;
    class Image;
}

namespace sauce {
namespace modeling {

struct LoadOptions {
    bool generateNormals = true;      // Generate normals if missing
    bool generateTangents = true;     // Generate tangents if missing
    bool validateMeshes = true;       // Validate mesh data
    bool loadTextures = true;         // Load texture data
};

class GLTFLoader {
public:
    GLTFLoader();
    explicit GLTFLoader(const LoadOptions& options);

    // Load all scenes from a GLTF file
    std::vector<std::shared_ptr<Model>> loadModels(const std::string& filePath);

    // Load a specific scene from a GLTF file (default: scene 0)
    std::shared_ptr<Model> loadModel(const std::string& filePath, size_t sceneIndex = 0);

    // Get load options
    const LoadOptions& getOptions() const { return options; }
    void setOptions(const LoadOptions& options) { this->options = options; }

    // Get texture cache
    TextureCache& getTextureCache() { return textureCache; }
    const TextureCache& getTextureCache() const { return textureCache; }

private:
    LoadOptions options;
    TextureCache textureCache;

    // Internal GLTF data during loading
    std::string baseDirectory;
    const tinygltf::Model* currentGltfModel;

    // Main processing functions
    bool loadGltfFile(const std::string& filePath, tinygltf::Model& gltfModel);
    std::shared_ptr<Model> processScene(const tinygltf::Model& gltfModel, int sceneIndex);

    // Node processing
    std::shared_ptr<ModelNode> processNode(const tinygltf::Model& gltfModel, int nodeIndex);
    void processNodeChildren(const tinygltf::Model& gltfModel,
                             const tinygltf::Node& gltfNode,
                             std::shared_ptr<ModelNode> node);

    // Mesh processing
    std::shared_ptr<Mesh> processPrimitive(const tinygltf::Model& gltfModel,
                                           const tinygltf::Primitive& primitive);
    void extractVertexAttribute(const tinygltf::Model& gltfModel,
                                const tinygltf::Primitive& primitive,
                                const std::string& attributeName,
                                std::vector<sauce::Vertex>& vertices,
                                int componentIndex);
    void extractIndices(const tinygltf::Model& gltfModel,
                       const tinygltf::Primitive& primitive,
                       std::vector<uint32_t>& indices);

    // Material processing
    std::shared_ptr<Material> processMaterial(const tinygltf::Model& gltfModel, int materialIndex);

    // Texture processing
    std::shared_ptr<Texture> processTexture(const tinygltf::Model& gltfModel,
                                            int textureIndex,
                                            TextureType type,
                                            bool sRGB);
    std::shared_ptr<Texture> processImage(const tinygltf::Model& gltfModel,
                                          int imageIndex,
                                          TextureType type,
                                          bool sRGB);

    // KHR_lights_punctual
    void parseLightsExtension(const tinygltf::Model& gltfModel);
    void applyNodeLight(const tinygltf::Node& gltfNode, std::shared_ptr<ModelNode> node);

    std::vector<LightInfo> parsedLights; // populated by parseLightsExtension

    // Helper functions
    Transform extractTransform(const tinygltf::Node& gltfNode);
    template<typename T>
    const T* getAccessorData(const tinygltf::Model& gltfModel, int accessorIndex, size_t& count);
};

} // namespace modeling
} // namespace sauce
