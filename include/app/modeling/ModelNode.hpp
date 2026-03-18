#pragma once

#include "app/modeling/Material.hpp"
#include "app/modeling/Mesh.hpp"
#include "app/modeling/PropertyValue.hpp"
#include "app/modeling/Transform.hpp"
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

namespace sauce {
    namespace modeling {

        struct MeshMaterialPair {
            std::shared_ptr<Mesh> mesh;
            std::shared_ptr<Material> material;
        };

        struct LightInfo {
            enum class Type {
                Directional = 0,
                Point = 1,
                Spot = 2
            };
            Type type{Type::Point};
            glm::vec3 color{1.0f};
            float intensity{1.0f};
            float range{0.0f};
            float innerConeAngle{0.0f};
            float outerConeAngle{0.7853981f}; // pi/4
            std::string name;
        };

        class ModelNode {
          public:
            ModelNode(const std::string& name = "");

            // Getters
            const std::string& getName() const {
                return name;
            }
            void setName(const std::string& name) {
                this->name = name;
            }

            Transform& getTransform() {
                return transform;
            }
            const Transform& getTransform() const {
                return transform;
            }

            ModelNode* getParent() const {
                return parent;
            }
            void setParent(ModelNode* parent) {
                this->parent = parent;
            }

            const std::vector<std::shared_ptr<ModelNode>>& getChildren() const {
                return children;
            }
            void addChild(std::shared_ptr<ModelNode> child);
            void removeChild(std::shared_ptr<ModelNode> child);

            const std::vector<MeshMaterialPair>& getMeshMaterialPairs() const {
                return meshMaterialPairs;
            }
            void addMeshMaterialPair(std::shared_ptr<Mesh> mesh,
                                     std::shared_ptr<Material> material);

            const std::optional<LightInfo>& getLightInfo() const {
                return lightInfo;
            }
            void setLightInfo(const LightInfo& info) {
                lightInfo = info;
            }
            bool hasLight() const {
                return lightInfo.has_value();
            }

            // Metadata access (for GLTF extensions)
            const std::unordered_map<std::string, PropertyValue>& getMetadata() const {
                return metadata;
            }
            void setMetadata(const std::string& key, const PropertyValue& value);
            bool hasMetadata(const std::string& key) const;

            // World transform computation
            glm::mat4 getWorldMatrix() const;

          private:
            std::string name;
            Transform transform;
            ModelNode* parent; // Raw pointer to avoid circular shared_ptr
            std::vector<std::shared_ptr<ModelNode>> children;
            std::vector<MeshMaterialPair> meshMaterialPairs;
            std::optional<LightInfo> lightInfo;
            std::unordered_map<std::string, PropertyValue> metadata;
        };

    } // namespace modeling
} // namespace sauce
