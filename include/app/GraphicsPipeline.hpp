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

  GraphicsPipeline(const sauce::LogicalDevice& logicalDevice, const vk::raii::DescriptorSetLayout& descriptorSetLayout, const sauce::SwapChain& swapChain, const std::string& shaderPath = "shaders/slang.spv") {
    vk::raii::ShaderModule shaderModule = createShaderModule(logicalDevice, readBinaryFile(shaderPath));
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

    initPipeline(logicalDevice, descriptorSetLayout, swapChain, shaderStages);
  }

  // Constructor for separate GLSL vertex and fragment shaders
  GraphicsPipeline(const sauce::LogicalDevice& logicalDevice, const vk::raii::DescriptorSetLayout& descriptorSetLayout, const sauce::SwapChain& swapChain, const std::string& vertShaderPath, const std::string& fragShaderPath) {
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

    initPipeline(logicalDevice, descriptorSetLayout, swapChain, shaderStages);
  }

private:
  vk::raii::PipelineLayout layout = nullptr;
  vk::raii::Pipeline pipeline = nullptr;
  vk::raii::ShaderModule vertShaderModule = nullptr;
  vk::raii::ShaderModule fragShaderModule = nullptr;

  void initPipeline(const sauce::LogicalDevice& logicalDevice, const vk::raii::DescriptorSetLayout& descriptorSetLayout, const sauce::SwapChain& swapChain, vk::PipelineShaderStageCreateInfo* shaderStages) {
    
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

    vk::PipelineRenderingCreateInfo renderingCreateInfo {
      .colorAttachmentCount = 1,
      .pColorAttachmentFormats = &swapChain.getSurfaceFormat().format,
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

};

}
