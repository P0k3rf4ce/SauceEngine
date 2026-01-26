#include "app/modeling/Model.hpp"
#include <unordered_set>

namespace sauce {
namespace modeling {

Model::Model() {
}

void Model::setRootNode(std::shared_ptr<ModelNode> root) {
    rootNode = root;
    rebuildFlatLists();
}

void Model::setMetadata(const std::string& key, const PropertyValue& value) {
    metadata[key] = value;
}

bool Model::hasMetadata(const std::string& key) const {
    return metadata.find(key) != metadata.end();
}

void Model::rebuildFlatLists() {
    allMeshes.clear();
    allMaterials.clear();

    if (rootNode) {
        traverseNode(rootNode);
    }
}

void Model::traverseNode(std::shared_ptr<ModelNode> node) {
    if (!node) {
        return;
    }

    // Use sets to avoid duplicates
    static std::unordered_set<std::shared_ptr<Mesh>> meshSet;
    static std::unordered_set<std::shared_ptr<Material>> materialSet;

    // Add meshes and materials from this node
    for (const auto& pair : node->getMeshMaterialPairs()) {
        if (pair.mesh && meshSet.find(pair.mesh) == meshSet.end()) {
            allMeshes.push_back(pair.mesh);
            meshSet.insert(pair.mesh);
        }
        if (pair.material && materialSet.find(pair.material) == materialSet.end()) {
            allMaterials.push_back(pair.material);
            materialSet.insert(pair.material);
        }
    }

    // Traverse children
    for (const auto& child : node->getChildren()) {
        traverseNode(child);
    }

    // Clear static sets after complete traversal
    if (node == rootNode) {
        meshSet.clear();
        materialSet.clear();
    }
}

} // namespace modeling
} // namespace sauce
