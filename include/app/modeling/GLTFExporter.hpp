#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace tinygltf {
    class Model;
}

namespace sauce {
class Scene;
class Entity;

namespace modeling {
class Mesh;
class Material;
class Texture;

struct ExportOptions {
    bool embedImages = false;
    bool embedBuffers = true;
    bool prettyPrint = true;
    bool writeBinary = false;
};

class GLTFExporter {
public:
    GLTFExporter();
    explicit GLTFExporter(const ExportOptions& options);

    bool exportScene(const Scene& scene, const std::string& filePath);

    const ExportOptions& getOptions() const { return options; }
    void setOptions(const ExportOptions& opts) { options = opts; }

private:
    ExportOptions options;

    // Deduplication maps (keyed on raw pointer from shared_ptr)
    std::unordered_map<const Mesh*, int> meshMap;
    std::unordered_map<const Material*, int> materialMap;
    std::unordered_map<const Texture*, int> textureMap;
    std::unordered_map<const Texture*, int> imageMap;

    // Single buffer for all vertex/index data
    std::vector<unsigned char> bufferData;

    std::string outputDirectory;

    void buildModel(tinygltf::Model& model, const Scene& scene);
    int processEntity(tinygltf::Model& model, const Entity& entity);
    int getOrCreateMesh(tinygltf::Model& model, const Entity& entity);
    int getOrCreateMaterial(tinygltf::Model& model, const Material& material);
    int getOrCreateTexture(tinygltf::Model& model, const Texture& texture);
    int getOrCreateImage(tinygltf::Model& model, const Texture& texture);

    int addAccessor(tinygltf::Model& model,
                    const void* data, size_t count,
                    int componentType, int type,
                    size_t byteStride,
                    const std::vector<double>& minValues = {},
                    const std::vector<double>& maxValues = {});

    void padBuffer(size_t alignment = 4);
};

} // namespace modeling
} // namespace sauce
