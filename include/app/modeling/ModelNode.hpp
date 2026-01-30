#pragma once

#include "app/modeling/Transform.hpp"
#include "app/modeling/Mesh.hpp"
#include "app/modeling/Material.hpp"
#include "app/modeling/PropertyValue.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace sauce {
namespace modeling {

struct MeshMaterialPair {
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material;
};

class ModelNode {
public:
    ModelNode(const std::string& name = "");

    // Getters
    const std::string& getName() const { return name; }
    void setName(const std::string& name) { this->name = name; }

    Transform& getTransform() { return transform; }
    const Transform& getTransform() const { return transform; }

    ModelNode* getParent() const { return parent; }
    void setParent(ModelNode* parent) { this->parent = parent; }

    const std::vector<std::shared_ptr<ModelNode>>& getChildren() const { return children; }
    void addChild(std::shared_ptr<ModelNode> child);
    void removeChild(std::shared_ptr<ModelNode> child);

    const std::vector<MeshMaterialPair>& getMeshMaterialPairs() const { return meshMaterialPairs; }
    void addMeshMaterialPair(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material);

    // Metadata access (for GLTF extensions)
    const std::unordered_map<std::string, PropertyValue>& getMetadata() const { return metadata; }
    void setMetadata(const std::string& key, const PropertyValue& value);
    bool hasMetadata(const std::string& key) const;

    // World transform computation
    glm::mat4 getWorldMatrix() const;

private:
    std::string name;
    Transform transform;
    ModelNode* parent;  // Raw pointer to avoid circular shared_ptr
    std::vector<std::shared_ptr<ModelNode>> children;
    std::vector<MeshMaterialPair> meshMaterialPairs;
    std::unordered_map<std::string, PropertyValue> metadata;
};

} // namespace modeling
} // namespace sauce
