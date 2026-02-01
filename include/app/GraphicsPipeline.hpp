#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <fstream>

#include <app/Vertex.hpp>
#include <app/LogicalDevice.hpp>
#include <app/SwapChain.hpp>

namespace sauce {

struct GraphicsPipeline {

  GraphicsPipeline(
      const sauce::PhysicalDevice& physicalDevice, 
      const sauce::LogicalDevice& logicalDevice, 
      const vk::raii::DescriptorSetLayout& descriptorSetLayout, 
      const sauce::SwapChain& swapChain
      ) {
    vk::raii::ShaderModule shaderModule = createShaderModule(logicalDevice, readBinaryFile("shaders/shader_lights.spv"));
    vk::PipelineShaderStageCreateInfo vertShaderCreateInfo {
      .stage = vk::ShaderStageFlagBits::eVertex,
      .module = shaderModule,
      .pName = "vertMain",
    };
    vk::PipelineShaderStageCreateInfo fragShaderCreateInfo {
      .stage = vk::ShaderStageFlagBits::eFragment,
      .module = shaderModule,
      .pName = "fragMain",
    };
    vk::PipelineShaderStageCreateInfo shaderStages[] = {
      vertShaderCreateInfo,
      fragShaderCreateInfo,
    };
    
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescription();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo {
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &bindingDescription,
      .vertexAttributeDescriptionCount = attributeDescriptions.size(),
      .pVertexAttributeDescriptions = attributeDescriptions.data(),
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo {
      .topology = vk::PrimitiveTopology::eTriangleList,
    };

    vk::PipelineViewportStateCreateInfo viewportStateInfo {
      .viewportCount = 1,
      .scissorCount = 1,
    };

    vk::PipelineRasterizationStateCreateInfo rasterizerInfo {
      .depthClampEnable = vk::False,
      .rasterizerDiscardEnable = vk::False,
      .polygonMode = vk::PolygonMode::eFill,
      .cullMode = vk::CullModeFlagBits::eBack,
      .frontFace = vk::FrontFace::eCounterClockwise,
      .depthBiasEnable = vk::False,
      .depthBiasSlopeFactor = 1.0f,
      .lineWidth = 1.0f,
    };

    vk::PipelineMultisampleStateCreateInfo multisamplingInfo {
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
      .sampleShadingEnable = vk::False,
    };


    vk::PipelineDepthStencilStateCreateInfo depthStencil {
      .depthTestEnable = vk::True,
      .depthWriteEnable = vk::True,
      .depthCompareOp = vk::CompareOp::eLess,
      .depthBoundsTestEnable = vk::False,
      .stencilTestEnable = vk::False,
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment {
      .blendEnable = vk::False,
      .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendInfo {
      .logicOpEnable = vk::False,
      .logicOp = vk::LogicOp::eCopy,
      .attachmentCount = 1,
      .pAttachments = &colorBlendAttachment,
    };

    std::vector<vk::DynamicState> dynamicStates {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor,
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateInfo {
      .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
      .pDynamicStates = dynamicStates.data(),
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
      .setLayoutCount = 1,
      .pSetLayouts = &*descriptorSetLayout,
      .pushConstantRangeCount = 0,
    };

    layout = vk::raii::PipelineLayout { *logicalDevice, pipelineLayoutInfo };


    vk::Format depthFormat = findDepthFormat(physicalDevice);

    vk::PipelineRenderingCreateInfo renderingCreateInfo {
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &swapChain.getSurfaceFormat().format,
      .depthAttachmentFormat = depthFormat,
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo {
      .pNext = &renderingCreateInfo,
      .stageCount = 2,
      .pStages = shaderStages,
      .pVertexInputState = &vertexInputInfo,
      .pInputAssemblyState = &inputAssemblyInfo,
      .pViewportState = &viewportStateInfo,
      .pRasterizationState = &rasterizerInfo,
      .pMultisampleState = &multisamplingInfo,
      .pDepthStencilState = &depthStencil,
      .pColorBlendState = &colorBlendInfo,
      .pDynamicState = &dynamicStateInfo,
      .layout = layout,
      .renderPass = nullptr,
    };

    pipeline = vk::raii::Pipeline { *logicalDevice, nullptr, pipelineInfo };
  }

  const vk::raii::Pipeline& operator*() const & noexcept {
    return pipeline;
  }

  const vk::raii::Pipeline* operator->() const & noexcept {
    return &pipeline;
  }

  const vk::raii::PipelineLayout& getLayout() const noexcept {
    return layout;
  }
  
  static vk::Format findDepthFormat(const sauce::PhysicalDevice& physicalDevice) {
    return findSupportedFormat(
        physicalDevice,
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
  }

private:
  vk::raii::PipelineLayout layout = nullptr;
  vk::raii::Pipeline pipeline = nullptr;

  static std::vector<char> readBinaryFile(const std::string filename) {
    std::ifstream file { filename, std::ios::binary | std::ios::ate };

    if (!file.is_open()) {
      throw std::runtime_error("Failed to open file: " + filename);
    }

    std::vector<char> buf ( file.tellg() );

    file.seekg(0, std::ios::beg);
    file.read(buf.data(), static_cast<std::streamsize>(buf.size()));

    file.close();

    return buf;
  }

  [[nodiscard]] vk::raii::ShaderModule createShaderModule(const sauce::LogicalDevice& logicalDevice, const std::vector<char>& code) const {
    vk::ShaderModuleCreateInfo shaderModuleCreateInfo {
      .codeSize = code.size() * sizeof(char),
      .pCode = reinterpret_cast<const uint32_t*>(code.data()),
    };

    return { *logicalDevice, shaderModuleCreateInfo };
  }


  static vk::Format findSupportedFormat(const sauce::PhysicalDevice& physicalDevice, const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
    for (const auto format: candidates) {
      vk::FormatProperties props = physicalDevice->getFormatProperties(format);
      if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features){
        return format;
      }
      if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
        return format;
      }
    }

    throw std::runtime_error("Failed to find supported format!");
  }

  bool hasStencilComponent(vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
  }

};

}
