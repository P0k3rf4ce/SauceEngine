#include "app/modeling/Model.hpp"
#include <unordered_set>

namespace sauce {
    namespace modeling {

        std::vector<MeshMaterialPair> Model::getAllMeshMaterialPairs() const {
            std::vector<MeshMaterialPair> allPairs;

            if (rootNode) {
                collectPairsFromNode(rootNode, allPairs);
            }

            return allPairs;
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

        void Model::collectPairsFromNode(std::shared_ptr<ModelNode> node,
                                         std::vector<MeshMaterialPair>& pairs) const {
            if (!node) {
                return;
            }

            // Add all pairs from this node
            for (const auto& pair : node->getMeshMaterialPairs()) {
                pairs.push_back(pair);
            }

            // Traverse children
            for (const auto& child : node->getChildren()) {
                collectPairsFromNode(child, pairs);
            }
        }

    } // namespace modeling
} // namespace sauce
