#pragma once

#include "app/modeling/ModelNode.hpp"
#include "app/modeling/Mesh.hpp"
#include "app/modeling/Material.hpp"
#include "app/modeling/PropertyValue.hpp"
#include <memory>
#include <vector>
#include <unordered_map>

namespace sauce {
namespace modeling {

class Model {
public:
    Model();

    // Root node access
    std::shared_ptr<ModelNode> getRootNode() const { return rootNode; }
    void setRootNode(std::shared_ptr<ModelNode> root);

    // Flat lists of all meshes and materials in the model
    const std::vector<std::shared_ptr<Mesh>>& getAllMeshes() const { return allMeshes; }
    const std::vector<std::shared_ptr<Material>>& getAllMaterials() const { return allMaterials; }

    // Metadata access (for scene-level GLTF extensions)
    const std::unordered_map<std::string, PropertyValue>& getMetadata() const { return metadata; }
    void setMetadata(const std::string& key, const PropertyValue& value);
    bool hasMetadata(const std::string& key) const;

    // Build flat lists by traversing the tree
    void rebuildFlatLists();

private:
    std::shared_ptr<ModelNode> rootNode;
    std::vector<std::shared_ptr<Mesh>> allMeshes;
    std::vector<std::shared_ptr<Material>> allMaterials;
    std::unordered_map<std::string, PropertyValue> metadata;

    // Helper for tree traversal
    void traverseNode(std::shared_ptr<ModelNode> node);
};

} // namespace modeling
} // namespace sauce
