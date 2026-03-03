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

struct GraphicsPipelineConfig {
  const sauce::PhysicalDevice& physicalDevice;
  const sauce::LogicalDevice& logicalDevice;
  const vk::raii::DescriptorSetLayout& descriptorSetLayout;
  vk::Format colorFormat;
  std::string shaderPath;
  const std::string vertEntryPoint = "vertMain";
  const std::string fragEntryPoint = "fragMain";
  bool hasVertexInput = true;
  bool enableBlending = false;
  bool enableCulling = true;
  bool depthWrite = true;
  bool depthTestEnable = true;
  bool hasPushConstants = false;
  uint32_t pushConstantSize = 0;
};

struct GraphicsPipeline {
  GraphicsPipelineConfig config;

  GraphicsPipeline(
      const sauce::GraphicsPipelineConfig& config
      ) : config(config) {
    vk::raii::ShaderModule shaderModule = createShaderModule(config.logicalDevice, readBinaryFile(config.shaderPath));
    vk::PipelineShaderStageCreateInfo vertShaderCreateInfo {
      .stage = vk::ShaderStageFlagBits::eVertex,
      .module = shaderModule,
      .pName = config.vertEntryPoint.c_str(),
    };
    vk::PipelineShaderStageCreateInfo fragShaderCreateInfo {
      .stage = vk::ShaderStageFlagBits::eFragment,
      .module = shaderModule,
      .pName = config.fragEntryPoint.c_str(),
    };
    vk::PipelineShaderStageCreateInfo shaderStages[] = {
      vertShaderCreateInfo,
      fragShaderCreateInfo,
    };

    initPipeline(shaderStages);
  }

  // Constructor for separate GLSL vertex and fragment shaders
  GraphicsPipeline(const sauce::GraphicsPipelineConfig& config, const std::string& vertShaderPath, const std::string& fragShaderPath) : config(config) {
    vertShaderModule = createShaderModule(config.logicalDevice, readBinaryFile(vertShaderPath));
    fragShaderModule = createShaderModule(config.logicalDevice, readBinaryFile(fragShaderPath));

    vk::PipelineShaderStageCreateInfo vertShaderCreateInfo {
      .stage = vk::ShaderStageFlagBits::eVertex,
      .module = vertShaderModule,
      .pName = config.vertEntryPoint.c_str(),
    };
    vk::PipelineShaderStageCreateInfo fragShaderCreateInfo {
      .stage = vk::ShaderStageFlagBits::eFragment,
      .module = fragShaderModule,
      .pName = config.fragEntryPoint.c_str(),
    };
    vk::PipelineShaderStageCreateInfo shaderStages[] = {
      vertShaderCreateInfo,
      fragShaderCreateInfo,
    };

    initPipeline(shaderStages);
  }

  // Constructor for editor pipelines with config (offscreen rendering)
  GraphicsPipeline(
      const sauce::PhysicalDevice& physicalDevice,
      const sauce::LogicalDevice& logicalDevice,
      const vk::raii::DescriptorSetLayout& descriptorSetLayout,
      vk::Format colorFormat,
      const std::string& vertShaderPath,
      const std::string& fragShaderPath,
      const GraphicsPipelineConfig& config
  ) : config(config) {
    vertShaderModule = createShaderModule(logicalDevice, readBinaryFile(vertShaderPath));
    fragShaderModule = createShaderModule(logicalDevice, readBinaryFile(fragShaderPath));

    vk::PipelineShaderStageCreateInfo vertShaderCreateInfo {
      .stage = vk::ShaderStageFlagBits::eVertex,
      .module = vertShaderModule,
      .pName = "main",
    };
    vk::PipelineShaderStageCreateInfo fragShaderCreateInfo {
      .stage = vk::ShaderStageFlagBits::eFragment,
      .module = fragShaderModule,
      .pName = "main",
    };
    vk::PipelineShaderStageCreateInfo shaderStages[] = {
      vertShaderCreateInfo,
      fragShaderCreateInfo,
    };

    initPipelineConfigurable(physicalDevice, logicalDevice, descriptorSetLayout, colorFormat, shaderStages, config);
  }


private:
  vk::raii::PipelineLayout layout = nullptr;
  vk::raii::Pipeline pipeline = nullptr;
  vk::raii::ShaderModule vertShaderModule = nullptr;
  vk::raii::ShaderModule fragShaderModule = nullptr;

  void initPipeline(vk::PipelineShaderStageCreateInfo* shaderStages) {
    
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescription();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo {
      .vertexBindingDescriptionCount = config.hasVertexInput ? 1u : 0u,
      .pVertexBindingDescriptions = config.hasVertexInput ? &bindingDescription : nullptr,
      .vertexAttributeDescriptionCount = config.hasVertexInput ? static_cast<uint32_t>(attributeDescriptions.size()) : 0u,
      .pVertexAttributeDescriptions = config.hasVertexInput ? attributeDescriptions.data() : nullptr,
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
      .depthTestEnable = config.depthTestEnable ? vk::True : vk::False,
      .depthWriteEnable = config.depthWrite ? vk::True : vk::False,
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

    vk::PushConstantRange pushConstantRange {
      .stageFlags = vk::ShaderStageFlagBits::eFragment,
      .offset = 0,
      .size = sizeof(uint32_t),
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
      .setLayoutCount = 1,
      .pSetLayouts = &*config.descriptorSetLayout,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &pushConstantRange,
    };

    layout = vk::raii::PipelineLayout { *config.logicalDevice, pipelineLayoutInfo };


    vk::Format depthFormat = findDepthFormat(config.physicalDevice);

    vk::PipelineRenderingCreateInfo renderingCreateInfo {
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &config.colorFormat,
      .depthAttachmentFormat = config.depthTestEnable ? depthFormat : vk::Format::eUndefined,
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

    pipeline = vk::raii::Pipeline { *config.logicalDevice, nullptr, pipelineInfo };
  }

  void initPipelineConfigurable(
      const sauce::PhysicalDevice& physicalDevice,
      const sauce::LogicalDevice& logicalDevice,
      const vk::raii::DescriptorSetLayout& descriptorSetLayout,
      vk::Format colorFormat,
      vk::PipelineShaderStageCreateInfo* shaderStages,
      const GraphicsPipelineConfig& config
  ) {
    // Vertex input - empty if no vertex input (e.g. grid fullscreen triangle)
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescription();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo {};
    if (config.hasVertexInput) {
      vertexInputInfo.vertexBindingDescriptionCount = 1;
      vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
      vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
      vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    }

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
      .cullMode = config.enableCulling ? vk::CullModeFlagBits::eBack : vk::CullModeFlagBits::eNone,
      .frontFace = vk::FrontFace::eCounterClockwise,
      .depthBiasEnable = vk::False,
      .lineWidth = 1.0f,
    };

    vk::PipelineMultisampleStateCreateInfo multisamplingInfo {
      .rasterizationSamples = vk::SampleCountFlagBits::e1,
      .sampleShadingEnable = vk::False,
    };

    vk::PipelineDepthStencilStateCreateInfo depthStencil {
      .depthTestEnable = config.depthTestEnable ? vk::True : vk::False,
      .depthWriteEnable = config.depthWrite ? vk::True : vk::False,
      .depthCompareOp = vk::CompareOp::eLessOrEqual,
      .depthBoundsTestEnable = vk::False,
      .stencilTestEnable = vk::False,
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    if (config.enableBlending) {
      colorBlendAttachment.blendEnable = vk::True;
      colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
      colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
      colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
      colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
      colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
      colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
    } else {
      colorBlendAttachment.blendEnable = vk::False;
    }

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

    vk::PushConstantRange pushRange {
      .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
      .offset = 0,
      .size = config.pushConstantSize,
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
      .setLayoutCount = 1,
      .pSetLayouts = &*descriptorSetLayout,
      .pushConstantRangeCount = config.hasPushConstants ? 1u : 0u,
      .pPushConstantRanges = config.hasPushConstants ? &pushRange : nullptr,
    };

    layout = vk::raii::PipelineLayout { *logicalDevice, pipelineLayoutInfo };

    vk::Format depthFormat = findDepthFormat(physicalDevice);

    vk::PipelineRenderingCreateInfo renderingCreateInfo {
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &colorFormat,
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


public:
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
