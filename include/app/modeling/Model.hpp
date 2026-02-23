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
    void setRootNode(std::shared_ptr<ModelNode> root) { rootNode = root; }

    const std::unordered_map<std::string, PropertyValue>& getMetadata() const { return metadata; }
    void setMetadata(const std::string& key, const PropertyValue& value) { metadata[key] = value; }
    bool hasMetadata(const std::string& key) const { return metadata.find(key) != metadata.end(); }

    void rebuildFlatLists() {
        meshMaterialPairs.clear();
        descriptorSets.clear();
        if (rootNode) traverseNode(rootNode);
    }

    void initVulkanResources(
        vk::raii::Device& device,
        vk::raii::PhysicalDevice& physicalDevice,
        vk::raii::DescriptorPool& pool,
        vk::raii::DescriptorSetLayout& layout)
    {
        for (auto& pair : meshMaterialPairs) {
            pair.mesh->initVulkanResources(device, physicalDevice);
            pair.material->initVulkanResources(device, physicalDevice);
        }

        std::vector<vk::DescriptorSetLayout> layouts(meshMaterialPairs.size(), *layout);
        
        vk::DescriptorSetAllocateInfo allocInfo{};
        allocInfo.descriptorPool = *pool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
        allocInfo.pSetLayouts = layouts.data();

        descriptorSets = device.allocateDescriptorSets(allocInfo);

        for (size_t i = 0; i < meshMaterialPairs.size(); ++i) {
            auto& material = meshMaterialPairs[i].material;

            std::vector<vk::WriteDescriptorSet> writes;

            auto bufferInfos = material->getDescriptorBufferInfos();
            auto imageInfos  = material->getDescriptorImageInfos();

            if (!bufferInfos.empty()) {
                writes.emplace_back(
                    *descriptorSets[i],
                    0,
                    0,
                    static_cast<uint32_t>(bufferInfos.size()),
                    vk::DescriptorType::eUniformBuffer,
                    nullptr,
                    bufferInfos.data()
                );
            }

            if (!imageInfos.empty()) {
                writes.emplace_back(
                    *descriptorSets[i],
                    1,
                    0,
                    static_cast<uint32_t>(imageInfos.size()),
                    vk::DescriptorType::eCombinedImageSampler,
                    imageInfos.data(),
                    nullptr
                );
            }

            device.updateDescriptorSets(writes, nullptr);
        }
    }

    void draw(vk::raii::CommandBuffer& buffer)
    {
        for (size_t i = 0; i < meshMaterialPairs.size(); ++i) {
            auto& pair = meshMaterialPairs[i];

            pair.mesh->bind(buffer);

            buffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics,
                pair.mesh->getPipelineLayout(),
                0,
                *descriptorSets[i],
                nullptr
            );

            pair.mesh->draw(buffer);
        }
    }

private:
    struct MeshMaterialPair {
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Material> material;
    };

    std::shared_ptr<ModelNode> rootNode;
    std::vector<MeshMaterialPair> meshMaterialPairs;
    std::vector<vk::raii::DescriptorSet> descriptorSets;
    std::unordered_map<std::string, PropertyValue> metadata;

    void traverseNode(std::shared_ptr<ModelNode> node)
    {
        if (!node) return;

        auto mesh = node->getMesh();
        auto material = node->getMaterial();

        if (mesh && material) {
            meshMaterialPairs.push_back({ mesh, material });
        }

        for (auto& child : node->getChildren()) {
            traverseNode(child);
        }
    }
};

} // namespace modeling
} // namespace sauce