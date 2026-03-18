#include "app/modeling/ModelNode.hpp"

namespace sauce {
    namespace modeling {

        ModelNode::ModelNode(const std::string& name) : name(name), parent(nullptr) {
        }

        void ModelNode::addChild(std::shared_ptr<ModelNode> child) {
            if (child) {
                children.push_back(child);
                child->setParent(this);
            }
        }

        void ModelNode::removeChild(std::shared_ptr<ModelNode> child) {
            auto it = std::find(children.begin(), children.end(), child);
            if (it != children.end()) {
                (*it)->setParent(nullptr);
                children.erase(it);
            }
        }

        void ModelNode::addMeshMaterialPair(std::shared_ptr<Mesh> mesh,
                                            std::shared_ptr<Material> material) {
            meshMaterialPairs.push_back({mesh, material});
        }

        void ModelNode::setMetadata(const std::string& key, const PropertyValue& value) {
            metadata[key] = value;
        }

        bool ModelNode::hasMetadata(const std::string& key) const {
            return metadata.find(key) != metadata.end();
        }

        glm::mat4 ModelNode::getWorldMatrix() const {
            glm::mat4 localMatrix = transform.getLocalMatrix();

            if (parent) {
                return parent->getWorldMatrix() * localMatrix;
            }

            return localMatrix;
        }

    } // namespace modeling
} // namespace sauce
