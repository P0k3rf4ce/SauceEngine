#pragma once

#include "app/components/LightComponent.hpp"
#include <glm/glm.hpp>
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan_raii.hpp>
#include <memory>

namespace sauce {
    struct LogicalDevice;
    struct PhysicalDevice;
}

namespace sauce {

class DirectionalLightComponent : public LightComponent {
public:
    DirectionalLightComponent();
    DirectionalLightComponent(const glm::vec3& color, float intensity);
    ~DirectionalLightComponent() override = default;

    const glm::vec3& getAmbient()   const { return ambient; }
    const glm::vec3& getDiffuse()   const { return diffuse; }
    const glm::vec3& getSpecular()  const { return specular; }

    void setAmbient(const glm::vec3& a)  { ambient = a; }
    void setDiffuse(const glm::vec3& d)  { diffuse = d; }
    void setSpecular(const glm::vec3& s) { specular = s; }

    GPULight toGPULight(const glm::vec3& worldPosition) const override;

    // Depth mapping resources
    static void initDescriptorSetLayout(const sauce::LogicalDevice& logicalDevice);
    static const vk::raii::DescriptorSetLayout& getDescriptorSetLayout();
    static void cleanup();

    bool hasDepthMappingResources() const { return (bool)depthDescriptorSet; }
    const vk::raii::DescriptorSet& getDepthDescriptorSet() const { return *depthDescriptorSet; }

    void initDepthMappingResources(
        const sauce::LogicalDevice& logicalDevice,
        const sauce::PhysicalDevice& physicalDevice,
        const vk::raii::CommandPool& commandPool,
        const vk::raii::Queue& queue,
        const vk::raii::DescriptorPool& descriptorPool,
        const vk::raii::DescriptorSetLayout& layout
    );

private:
    glm::vec3 ambient{0.1f};
    glm::vec3 diffuse{1.0f};
    glm::vec3 specular{1.0f};

    static std::unique_ptr<vk::raii::DescriptorSetLayout> descriptorSetLayout;

    std::unique_ptr<vk::raii::Image> depthImage;
    std::unique_ptr<vk::raii::DeviceMemory> depthImageMemory;
    std::unique_ptr<vk::raii::ImageView> depthImageView;
    std::unique_ptr<vk::raii::Sampler> depthSampler;
    std::unique_ptr<vk::raii::DescriptorSet> depthDescriptorSet;
};

} // namespace sauce
