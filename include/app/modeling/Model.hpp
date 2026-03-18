#pragma once

#include "app/LogicalDevice.hpp"
#include "app/modeling/Material.hpp"
#include "app/modeling/Mesh.hpp"
#include "app/modeling/ModelNode.hpp"
#include "app/modeling/PropertyValue.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace sauce {
    namespace modeling {

        class Model {
          public:
            Model() = default;

            std::shared_ptr<ModelNode> getRootNode() const {
                return rootNode;
            }
            void setRootNode(std::shared_ptr<ModelNode> root) {
                rootNode = root;
                rebuildFlatLists();
            }

            // Flat lists of all meshes and materials in the model
            const std::vector<std::shared_ptr<Mesh>>& getAllMeshes() const {
                return allMeshes;
            }
            const std::vector<std::shared_ptr<Material>>& getAllMaterials() const {
                return allMaterials;
            }

            // Get all mesh-material pairs by traversing the scene graph
            std::vector<MeshMaterialPair> getAllMeshMaterialPairs() const;

            // Metadata access (for scene-level GLTF extensions)
            const std::unordered_map<std::string, PropertyValue>& getMetadata() const {
                return metadata;
            }
            void setMetadata(const std::string& key, const PropertyValue& value) {
                metadata[key] = value;
            }
            bool hasMetadata(const std::string& key) const {
                return metadata.find(key) != metadata.end();
            }

            void rebuildFlatLists() {
                allMeshes.clear();
                allMaterials.clear();
                descriptorSets.clear();
                if (rootNode)
                    traverseNode(rootNode);
            }

            void initVulkanResources(const sauce::LogicalDevice& logicalDevice,
                                     vk::raii::PhysicalDevice& physicalDevice,
                                     vk::raii::CommandPool& commandPool, vk::raii::Queue& queue,
                                     vk::raii::DescriptorPool& pool,
                                     const vk::raii::ImageView& defaultView,
                                     const vk::raii::Sampler& defaultSampler) {
                for (auto& mesh : allMeshes) {
                    mesh->initVulkanResources(logicalDevice, physicalDevice, commandPool, queue);
                }

                for (auto& material : allMaterials) {
                    material->initVulkanResources(logicalDevice, physicalDevice, commandPool, queue,
                                                  pool, defaultView, defaultSampler);
                }

                if (allMaterials.empty())
                    return;

                // Model-level descriptor sets are now redundant as Materials manage their own,
                // but we'll keep the allocation logic if it's expected by other parts of the engine.
                // However, we should use Material::getDescriptorSetLayout() for consistency.

                std::vector<vk::DescriptorSetLayout> layouts(allMaterials.size(),
                                                             *Material::getDescriptorSetLayout());

                vk::DescriptorSetAllocateInfo allocInfo{
                    .descriptorPool = *pool,
                    .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
                    .pSetLayouts = layouts.data(),
                };

                descriptorSets = logicalDevice->allocateDescriptorSets(allocInfo);

                for (size_t i = 0; i < allMaterials.size(); ++i) {
                    auto& material = allMaterials[i];

                    std::vector<vk::WriteDescriptorSet> writes;

                    auto bufferInfos = material->getDescriptorBufferInfos();
                    auto imageInfos =
                        material->getDescriptorImageInfos(defaultView, defaultSampler);

                    if (!bufferInfos.empty()) {
                        writes.emplace_back(vk::WriteDescriptorSet{
                            .dstSet = *descriptorSets[i],
                            .dstBinding = 5, // Material Properties UBO is binding 5
                            .dstArrayElement = 0,
                            .descriptorCount = static_cast<uint32_t>(bufferInfos.size()),
                            .descriptorType = vk::DescriptorType::eUniformBuffer,
                            .pBufferInfo = bufferInfos.data(),
                        });
                    }

                    if (!imageInfos.empty()) {
                        writes.emplace_back(vk::WriteDescriptorSet{
                            .dstSet = *descriptorSets[i],
                            .dstBinding = 0, // Albedo, Normal, etc. start at binding 0
                            .dstArrayElement = 0,
                            .descriptorCount = static_cast<uint32_t>(imageInfos.size()),
                            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                            .pImageInfo = imageInfos.data(),
                        });
                    }

                    logicalDevice->updateDescriptorSets(writes, nullptr);
                }
            }

            void draw(vk::raii::CommandBuffer& buffer,
                      const vk::raii::PipelineLayout& pipelineLayout) {
                // Model::draw is currently only used if we draw all materials.
                // But in recordSceneCommandBuffer, we draw per-entity.
                // If we use this draw, we bind Set Index 1.
                for (size_t i = 0; i < allMaterials.size(); ++i) {
                    // This is a bit problematic because Model doesn't know which mesh belongs to which material here
                    // unless they are 1:1.
                    // Let's assume they are 1:1 for now as per traverseNode.
                    if (i < allMeshes.size()) {
                        auto& mesh = allMeshes[i];
                        mesh->bind(buffer);

                        buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout,
                                                  2, // Set Index 2: Material
                                                  *descriptorSets[i], nullptr);

                        mesh->draw(buffer);
                    }
                }
            }

            const vk::raii::DescriptorSet& getDescriptorSet(size_t index) const {
                return descriptorSets[index];
            }

          private:
            std::shared_ptr<ModelNode> rootNode;
            std::vector<std::shared_ptr<Mesh>> allMeshes;
            std::vector<std::shared_ptr<Material>> allMaterials;
            std::vector<vk::raii::DescriptorSet> descriptorSets;
            std::unordered_map<std::string, PropertyValue> metadata;

            void traverseNode(std::shared_ptr<ModelNode> node);
            void collectPairsFromNode(std::shared_ptr<ModelNode> node,
                                      std::vector<MeshMaterialPair>& pairs) const;
        };

    } // namespace modeling
} // namespace sauce
