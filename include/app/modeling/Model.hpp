#pragma once

#include "app/modeling/ModelNode.hpp"
#include "app/modeling/Mesh.hpp"
#include "app/modeling/Material.hpp"
#include "app/modeling/PropertyValue.hpp"
#include <vulkan/vulkan_raii.hpp> // ??
#include <memory>
#include <vector>
#include <unordered_map>

namespace sauce {
namespace modeling {

class Model {
public:
    Model() = default;

    std::shared_ptr<ModelNode> getRootNode() const { return rootNode; }
    void setRootNode(std::shared_ptr<ModelNode> root) { rootNode = root; rebuildFlatLists(); }

    // Flat lists of all meshes and materials in the model
    const std::vector<std::shared_ptr<Mesh>>& getAllMeshes() const { return allMeshes; }
    const std::vector<std::shared_ptr<Material>>& getAllMaterials() const { return allMaterials; }

    const std::unordered_map<std::string, PropertyValue>& getMetadata() const { return metadata; }
    void setMetadata(const std::string& key, const PropertyValue& value) { metadata[key] = value; }
    bool hasMetadata(const std::string& key) const { return metadata.find(key) != metadata.end(); }

    void rebuildFlatLists() {
        allMeshes.clear();
        allMaterials.clear();
        descriptorSets.clear();
        if (rootNode) traverseNode(rootNode);
    }

    // This assumes that the flat lists are populated
    void initVulkanResources(
        vk::raii::Device& device,
        vk::raii::PhysicalDevice& physicalDevice,
        vk::raii::DescriptorPool& pool,
        vk::raii::DescriptorSetLayout& layout)
    {
        for (auto& mesh : allMeshes) {
            mesh->initVulkanResources(device, physicalDevice);
        }
        for (auto& material: allMaterials) {
          material->initVulkanResources(device, physicalDevice);
        }

        std::vector<vk::DescriptorSetLayout> layouts(allMeshes.size(), *layout);
        
        vk::DescriptorSetAllocateInfo allocInfo{
          .descriptorPool = *pool,
          .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
          .pSetLayouts = layouts.data(),
        };

        descriptorSets = device.allocateDescriptorSets(allocInfo);

        for (size_t i = 0; i < allMaterials.size(); ++i) {
            auto& material = allMaterials[i];

            std::vector<vk::WriteDescriptorSet> writes;

            auto bufferInfos = material->getDescriptorBufferInfos();
            auto imageInfos  = material->getDescriptorImageInfos();

            if (!bufferInfos.empty()) {
                writes.emplace_back(vk::WriteDescriptorSet {
                  .dstSet = *descriptorSets[i],
                  .dstBinding = 0,
                  .dstArrayElement = 0,
                  .descriptorCount = static_cast<uint32_t>(bufferInfos.size()),
                  .descriptorType = vk::DescriptorType::eUniformBuffer,
                  .pBufferInfo = bufferInfos.data(),
                });
            }

            //if (!imageInfos.empty()) {
            //    writes.emplace_back(vk::WriteDescriptorSet {
            //      .dstSet = *descriptorSets[i],
            //      .dstBinding = 1,
            //      .dstArrayElement = 0,
            //      .descriptorCount = static_cast<uint32_t>(imageInfos.size()),
            //      .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            //      .pBufferInfo = imageInfos.data(),
            //    });
            //}

            device.updateDescriptorSets(writes, nullptr);
        }
    }

    void draw(vk::raii::CommandBuffer& buffer)
    {
        for (size_t i = 0; i < allMaterials.size(); ++i) {
            auto& material = allMaterials[i];
            auto& mesh = allMeshes[i];

            mesh->bind(buffer);

            buffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                mesh->getPipelineLayout(),
                0,
                *descriptorSets[i],
                nullptr
            );

            mesh->draw(buffer);
        }
    }

private:
    std::shared_ptr<ModelNode> rootNode;
    std::vector<std::shared_ptr<Mesh>> allMeshes;
    std::vector<std::shared_ptr<Material>> allMaterials; // Same size as allMeshes
    std::vector<vk::raii::DescriptorSet> descriptorSets;
    std::unordered_map<std::string, PropertyValue> metadata;

    void traverseNode(std::shared_ptr<ModelNode> node);
};

} // namespace modeling
} // namespace sauce
