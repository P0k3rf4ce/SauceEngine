#include <editor/gizmos/GizmoRenderer.hpp>
#include <editor/EditorApp.hpp>
#include <editor/OffscreenFramebuffer.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace sauce::editor {

GizmoRenderer::GizmoRenderer(
    sauce::PhysicalDevice& physicalDevice,
    const sauce::LogicalDevice& logicalDevice,
    const vk::raii::DescriptorSetLayout& descriptorSetLayout,
    vk::Format colorFormat,
    sauce::Renderer& renderer)
  : pPhysicalDevice(&physicalDevice)
  , pLogicalDevice(&logicalDevice)
  , pRenderer(&renderer)
{
  sauce::GraphicsPipelineConfig gizmoConfig {
    .physicalDevice = physicalDevice,
    .logicalDevice = logicalDevice,
    .descriptorSetLayouts = { *descriptorSetLayout },
    .colorFormat = sauce::editor::OffscreenFramebuffer::COLOR_FORMAT,
    .hasVertexInput = true,
    .enableBlending = false,
    .enableCulling = false,
    .depthWrite = false,
    .depthTestEnable = false,
    .hasPushConstants = true,
    .pushConstantSize = sizeof(MeshPushConstants),
  };

  pipeline = std::make_unique<sauce::GraphicsPipeline>(
    physicalDevice, logicalDevice,
    gizmoConfig.descriptorSetLayouts,
    colorFormat,
    "shaders/editor_unlit.vert.spv",
    "shaders/editor_unlit.frag.spv",
    gizmoConfig
  );

  activeGizmo = std::make_unique<TranslateGizmo>();
  meshDirty = true;
}

void GizmoRenderer::setActiveGizmo(GizmoType type) {
  if (type == activeType && activeGizmo) return;
  activeType = type;

  switch (type) {
    case GizmoType::Translate:
      activeGizmo = std::make_unique<TranslateGizmo>();
      break;
    case GizmoType::Rotate:
      activeGizmo = std::make_unique<RotateGizmo>();
      break;
    case GizmoType::Scale:
      activeGizmo = std::make_unique<ScaleGizmo>();
      break;
  }
  meshDirty = true;
}

void GizmoRenderer::uploadMesh() {
  if (!activeGizmo) return;

  // Wait for GPU to finish using the old mesh before destroying it
  if (mesh) {
    (*pLogicalDevice)->waitIdle();
  }

  GizmoMeshData meshData = activeGizmo->generateMesh();
  mesh = std::make_unique<sauce::modeling::Mesh>(meshData.vertices, meshData.indices);

  auto& physDev = **pPhysicalDevice;
  auto& cmdPool = pRenderer->getCommandPool();
  auto& queue = pRenderer->getQueue();
  mesh->initVulkanResources(*pLogicalDevice, physDev, cmdPool, queue);

  meshDirty = false;
}

void GizmoRenderer::render(
    vk::raii::CommandBuffer& cmd,
    const vk::raii::DescriptorSet& descriptorSet,
    const glm::vec3& entityPosition,
    const EditorCamera& camera,
    float aspect)
{
  if (!activeGizmo) return;
  if (meshDirty) uploadMesh();
  if (!mesh || !mesh->hasGPUData()) return;

  // Constant screen size: scale by distance to camera
  float dist = glm::length(camera.getPosition() - entityPosition);
  float gizmoScale = dist * SCALE_FACTOR;

  glm::mat4 model = glm::translate(glm::mat4(1.0f), entityPosition);
  model = glm::scale(model, glm::vec3(gizmoScale));

  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, **pipeline);
  cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
    pipeline->getLayout(), 0, *descriptorSet, nullptr);
  MeshPushConstants pushData {};
  pushData.model = model;
  pushData.baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
  pushData.metallic = 0.0f;
  pushData.roughness = 0.0f;
  cmd.pushConstants<MeshPushConstants>(pipeline->getLayout(),
    vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushData);

  mesh->bind(cmd);
  mesh->draw(cmd);
}

} // namespace sauce::editor
