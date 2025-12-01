#ifndef MODEL_HPP
#define MODEL_HPP

#include "modeling/Mesh.hpp"
#include "modeling/Material.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <variant>
#include <string>
#include <utility>

namespace modeling {

// PropertyValue type for storing metadata
using PropertyValue = std::variant<int, bool, float, double, std::string>;

// A mesh paired with its associated material
using MeshMaterialPair = std::pair<std::shared_ptr<Mesh>, std::shared_ptr<Material>>;

class Model {
public:
    Model();

    Model(std::vector<MeshMaterialPair> meshMaterials, std::shared_ptr<Shader> shader);

    ~Model();

    // Get all mesh-material pairs
    const std::vector<MeshMaterialPair>& getMeshMaterialPairs() const;

    // Add a mesh with its associated material
    void addMesh(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);

    std::shared_ptr<Shader> getShader();

    // Metadata management
    void setMetadata(const std::unordered_map<std::string, PropertyValue>& data);
    const std::unordered_map<std::string, PropertyValue>& getMetadata() const;

    template<typename T>
    void setMetadataValue(const std::string& key, const T& value) {
        metadata[key] = value;
    }

    template<typename T>
    T getMetadataValue(const std::string& key) const {
        auto it = metadata.find(key);
        if (it != metadata.end()) {
            return std::get<T>(it->second);
        }
        throw std::runtime_error("Metadata key not found: " + key);
    }

    bool hasMetadata(const std::string& key) const {
        return metadata.find(key) != metadata.end();
    }

    // Rendering methods
    void setupForRendering();  // Prepare all meshes and bind shader

private:
    std::vector<MeshMaterialPair> meshMaterials;
    std::shared_ptr<Shader> shader;
    std::unordered_map<std::string, PropertyValue> metadata;
};

} // namespace modeling

#endif // MODEL_HPP
