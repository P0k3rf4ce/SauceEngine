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

// Matches the PointLight struct in shader_lights.slang (std140/ConstantBuffer layout, 96 bytes).
// Used to populate ConstantBuffer<PointLight[N]> UBOs for the Blinn-Phong lighting path.
struct alignas(16) GPUPointLight {
    glm::vec3 position{0.0f};          // offset  0
    float     _pad0{0};                // offset 12
    glm::vec3 color{1.0f};             // offset 16
    float     _pad1{0};                // offset 28
    glm::vec3 ambient{0.05f};          // offset 32
    float     _pad2{0};                // offset 44
    glm::vec3 diffuse{0.8f};           // offset 48
    float     _pad3{0};                // offset 60
    glm::vec3 specular{1.0f};          // offset 64
    float     constant{1.0f};          // offset 76
    float     linear{0.09f};           // offset 80
    float     quadratic{0.032f};       // offset 84
    float     _pad4{0};                // offset 88
};
static_assert(sizeof(GPUPointLight) == 96, "GPUPointLight must be 96 bytes to match shader_lights.slang");

class PointLightComponent : public LightComponent {
public:
    PointLightComponent();
    PointLightComponent(const glm::vec3& color, float intensity, float range = 0.0f);
    ~PointLightComponent() override = default;

    // PBR properties (shader_pbr.slang path)
    float getRange() const { return range; }
    void  setRange(float r) { range = r; }

    // Blinn-Phong properties (shader_lights.slang path)
    const glm::vec3& getAmbient()   const { return ambient; }
    const glm::vec3& getDiffuse()   const { return diffuse; }
    const glm::vec3& getSpecular()  const { return specular; }
    float getConstant()  const { return constant; }
    float getLinear()    const { return linear; }
    float getQuadratic() const { return quadratic; }

    void setAmbient(const glm::vec3& a)  { ambient = a; }
    void setDiffuse(const glm::vec3& d)  { diffuse = d; }
    void setSpecular(const glm::vec3& s) { specular = s; }
    void setConstant(float c)  { constant = c; }
    void setLinear(float l)    { linear = l; }
    void setQuadratic(float q) { quadratic = q; }

    void setAttenuation(float c, float l, float q) {
        constant = c; linear = l; quadratic = q;
    }

    GPUPointLight toGPUPointLight(const glm::vec3& worldPosition) const;
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
    float range{0.0f};

    glm::vec3 ambient{0.05f};
    glm::vec3 diffuse{0.8f};
    glm::vec3 specular{1.0f};
    float constant{1.0f};
    float linear{0.09f};
    float quadratic{0.032f};

    static std::unique_ptr<vk::raii::DescriptorSetLayout> descriptorSetLayout;

    std::unique_ptr<vk::raii::Image> depthImage;
    std::unique_ptr<vk::raii::DeviceMemory> depthImageMemory;
    std::unique_ptr<vk::raii::ImageView> depthImageView;
    std::unique_ptr<vk::raii::Sampler> depthSampler;
    std::unique_ptr<vk::raii::DescriptorSet> depthDescriptorSet;
};

} // namespace sauce
