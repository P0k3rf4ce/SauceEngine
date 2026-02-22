#include <app/ImGuiRenderer.hpp>
#include <app/SwapChain.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <stdexcept>

namespace sauce {

ImGuiRenderer::ImGuiRenderer(const ImGuiRendererCreateInfo& createInfo) {
  window = createInfo.window;
  device = &*createInfo.logicalDevice;

  createDescriptorPool(createInfo.logicalDevice);
  initImGui(createInfo);
  uploadFonts(createInfo);
}

ImGuiRenderer::~ImGuiRenderer() {
  if (imguiContext) {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(imguiContext);
  }
}

void ImGuiRenderer::createDescriptorPool(const sauce::LogicalDevice& logicalDevice) {
  // Descriptor pool with eFreeDescriptorSet flag for dynamic descriptor allocation
  std::array<vk::DescriptorPoolSize, 11> poolSizes = {{
    { vk::DescriptorType::eSampler, 1000 },
    { vk::DescriptorType::eCombinedImageSampler, 1000 },
    { vk::DescriptorType::eSampledImage, 1000 },
    { vk::DescriptorType::eStorageImage, 1000 },
    { vk::DescriptorType::eUniformTexelBuffer, 1000 },
    { vk::DescriptorType::eStorageTexelBuffer, 1000 },
    { vk::DescriptorType::eUniformBuffer, 1000 },
    { vk::DescriptorType::eStorageBuffer, 1000 },
    { vk::DescriptorType::eUniformBufferDynamic, 1000 },
    { vk::DescriptorType::eStorageBufferDynamic, 1000 },
    { vk::DescriptorType::eInputAttachment, 1000 }
  }};

  vk::DescriptorPoolCreateInfo poolInfo{
    .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
    .maxSets = 1000,
    .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
    .pPoolSizes = poolSizes.data()
  };

  imguiDescriptorPool = vk::raii::DescriptorPool{ *logicalDevice, poolInfo };
}

void ImGuiRenderer::initImGui(const ImGuiRendererCreateInfo& createInfo) {
  // Create ImGui context
  imguiContext = ImGui::CreateContext();
  ImGui::SetCurrentContext(imguiContext);

  // Setup ImGui style
  ImGui::StyleColorsDark();

  // Initialize GLFW backend
  ImGui_ImplGlfw_InitForVulkan(window, true);

  // Initialize Vulkan backend with dynamic rendering
  VkFormat colorAttachmentFormat = static_cast<VkFormat>(createInfo.swapChainFormat);

  VkFormat depthFormat = static_cast<VkFormat>(createInfo.depthFormat);

  VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo{};
  pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
  pipelineRenderingCreateInfo.colorAttachmentCount = 1;
  pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorAttachmentFormat;
  pipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;

  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.Instance = *createInfo.instance;
  initInfo.PhysicalDevice = **createInfo.physicalDevice;
  initInfo.Device = **createInfo.logicalDevice;
  initInfo.QueueFamily = createInfo.queueFamilyIndex;
  initInfo.Queue = *createInfo.queue;
  initInfo.DescriptorPool = *imguiDescriptorPool;
  initInfo.MinImageCount = createInfo.imageCount;
  initInfo.ImageCount = createInfo.imageCount;
  initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  initInfo.UseDynamicRendering = true;  // Enable dynamic rendering
  initInfo.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo;

  if (!ImGui_ImplVulkan_Init(&initInfo)) {
    throw std::runtime_error("Failed to initialize ImGui Vulkan backend");
  }
}

void ImGuiRenderer::uploadFonts(const ImGuiRendererCreateInfo& createInfo) {
  // Upload fonts to GPU using a one-time command buffer
  vk::CommandBufferAllocateInfo allocInfo{
    .commandPool = *createInfo.commandPool,
    .level = vk::CommandBufferLevel::ePrimary,
    .commandBufferCount = 1
  };

  vk::raii::CommandBuffer commandBuffer =
    std::move(createInfo.logicalDevice->allocateCommandBuffers(allocInfo).front());

  commandBuffer.begin(vk::CommandBufferBeginInfo{
    .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
  });

  ImGui_ImplVulkan_CreateFontsTexture();

  commandBuffer.end();

  createInfo.queue.submit(vk::SubmitInfo{
    .commandBufferCount = 1,
    .pCommandBuffers = &*commandBuffer
  }, nullptr);

  createInfo.queue.waitIdle();

  // Clean up font upload resources
  ImGui_ImplVulkan_DestroyFontsTexture();
}

void ImGuiRenderer::newFrame() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiRenderer::render(const vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex) {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *commandBuffer);
}

} // namespace sauce
