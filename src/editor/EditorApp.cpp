#include <editor/EditorApp.hpp>
#include <editor/panels/SceneHierarchyPanel.hpp>
#include <editor/panels/InspectorPanel.hpp>
#include <editor/panels/ViewportPanel.hpp>
#include <editor/panels/AssetBrowserPanel.hpp>
#include <editor/AABB.hpp>
#include <editor/gizmos/GizmoRenderer.hpp>
#include <app/GraphicsPipeline.hpp>
#include <app/components/TransformComponent.hpp>
#include <app/components/MeshRendererComponent.hpp>
#include <app/modeling/Material.hpp>

#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <filesystem>
#include <cmath>
#include <cstring>

namespace sauce::editor {

static constexpr uint32_t EDITOR_WIDTH = 1920;
static constexpr uint32_t EDITOR_HEIGHT = 1080;

EditorApp::EditorApp() = default;

EditorApp::~EditorApp() {
  if (pRenderer) {
    logicalDevice->waitIdle();
  }

  hierarchyPanel.reset();
  inspectorPanel.reset();
  viewportPanel.reset();
  assetBrowserPanel.reset();

  pGizmoRenderer.reset();
  pGridPipeline.reset();
  pUnlitPipeline.reset();
  pLitPipeline.reset();
  pOffscreenFB.reset();

  pImGuiRenderer.reset();
  pRenderer.reset();
  pScene.reset();

  if (window) {
    glfwDestroyWindow(window);
    glfwTerminate();
  }
}

void EditorApp::run() {
  initWindow();
  initVulkan();
  initEditor();
  mainLoop();
}

void EditorApp::initWindow() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window = glfwCreateWindow(EDITOR_WIDTH, EDITOR_HEIGHT, "SauceEditor", nullptr, nullptr);

  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
  glfwSetMouseButtonCallback(window, mouseButtonCallback);
  glfwSetCursorPosCallback(window, cursorPosCallback);
  glfwSetScrollCallback(window, scrollCallback);
  glfwSetKeyCallback(window, keyCallback);
  glfwSetDropCallback(window, dropCallback);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void EditorApp::framebufferResizeCallback(GLFWwindow* window, int /*width*/, int /*height*/) {
  auto* app = static_cast<EditorApp*>(glfwGetWindowUserPointer(window));
  if (app && app->pRenderer) {
    app->pRenderer->setFramebufferResized();
  }
}

void EditorApp::initVulkan() {
  uint32_t glfwExtensionsCount = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
  pInstance = std::make_unique<sauce::Instance>(glfwExtensions, glfwExtensionsCount);

  pRenderSurface = std::make_unique<sauce::RenderSurface>(*pInstance, window);

  physicalDevice = { *pInstance };
  logicalDevice = { physicalDevice, *pRenderSurface };

  sauce::CameraCreateInfo cameraCreateInfo {
    .scrWidth = static_cast<float>(EDITOR_WIDTH),
    .scrHeight = static_cast<float>(EDITOR_HEIGHT),
  };

  pScene = std::make_unique<sauce::Scene>(cameraCreateInfo);

  sauce::RendererCreateInfo rendererCreateInfo {
    .physicalDevice = physicalDevice,
    .logicalDevice = logicalDevice,
    .renderSurface = *pRenderSurface,
    .window = window,
  };

  pRenderer = std::make_unique<sauce::Renderer>(rendererCreateInfo);

  sauce::ImGuiRendererCreateInfo imguiCreateInfo {
    .instance = **pInstance,
    .physicalDevice = physicalDevice,
    .logicalDevice = logicalDevice,
    .queueFamilyIndex = logicalDevice.getQueueIndex(),
    .window = window,
    .queue = pRenderer->getQueue(),
    .commandPool = pRenderer->getCommandPool(),
    .swapChain = pRenderer->getSwapChain(),
    .imageCount = static_cast<uint32_t>(pRenderer->getSwapChain().getImages().size()),
    .swapChainFormat = pRenderer->getSwapChain().getSurfaceFormat().format,
    .depthFormat = sauce::GraphicsPipeline::findDepthFormat(physicalDevice),
  };

  pImGuiRenderer = std::make_unique<sauce::ImGuiRenderer>(imguiCreateInfo);

  // Enable docking
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  setupEditorTheme();

  // Create offscreen framebuffer for viewport rendering
  pOffscreenFB = std::make_unique<OffscreenFramebuffer>(
    physicalDevice, logicalDevice, 800, 600
  );

  // Create grid pipeline (no vertex input, alpha blending, no culling, no depth write)
  sauce::GraphicsPipelineConfig gridConfig {
    .hasVertexInput = false,
    .enableBlending = true,
    .enableCulling = false,
    .depthWrite = false,
    .hasPushConstants = false,
    .pushConstantSize = 0,
  };
  pGridPipeline = std::make_unique<sauce::GraphicsPipeline>(
    physicalDevice, logicalDevice,
    pRenderer->getDescriptorSetLayout(),
    OffscreenFramebuffer::COLOR_FORMAT,
    "shaders/editor_grid.vert.spv",
    "shaders/editor_grid.frag.spv",
    gridConfig
  );

  // Create unlit pipeline (vertex input, no blending, culling, depth write, push constants)
  sauce::GraphicsPipelineConfig unlitConfig {
    .hasVertexInput = true,
    .enableBlending = false,
    .enableCulling = true,
    .depthWrite = true,
    .hasPushConstants = true,
    .pushConstantSize = sizeof(MeshPushConstants),
  };
  pUnlitPipeline = std::make_unique<sauce::GraphicsPipeline>(
    physicalDevice, logicalDevice,
    pRenderer->getDescriptorSetLayout(),
    OffscreenFramebuffer::COLOR_FORMAT,
    "shaders/editor_unlit.vert.spv",
    "shaders/editor_unlit.frag.spv",
    unlitConfig
  );

  // Create lit pipeline (same config, PBR shaders)
  sauce::GraphicsPipelineConfig litConfig {
    .hasVertexInput = true,
    .enableBlending = false,
    .enableCulling = true,
    .depthWrite = true,
    .hasPushConstants = true,
    .pushConstantSize = sizeof(MeshPushConstants),
  };
  pLitPipeline = std::make_unique<sauce::GraphicsPipeline>(
    physicalDevice, logicalDevice,
    pRenderer->getDescriptorSetLayout(),
    OffscreenFramebuffer::COLOR_FORMAT,
    "shaders/editor_lit.vert.spv",
    "shaders/editor_lit.frag.spv",
    litConfig
  );

  // Create gizmo renderer
  pGizmoRenderer = std::make_unique<GizmoRenderer>(
    physicalDevice, logicalDevice,
    pRenderer->getDescriptorSetLayout(),
    OffscreenFramebuffer::COLOR_FORMAT,
    *pRenderer
  );

  // Set up custom command buffer recording for the editor
  pRenderer->setCommandBufferRecorder(
    [this](vk::raii::CommandBuffer& cmd, uint32_t imageIndex) {
      recordEditorCommandBuffer(cmd, imageIndex);
    }
  );
}

void EditorApp::setupEditorTheme() {
  ImGuiStyle& style = ImGui::GetStyle();

  // Rounding
  style.WindowRounding = 4.0f;
  style.ChildRounding = 2.0f;
  style.FrameRounding = 3.0f;
  style.PopupRounding = 3.0f;
  style.ScrollbarRounding = 3.0f;
  style.GrabRounding = 2.0f;
  style.TabRounding = 3.0f;

  // Spacing
  style.WindowPadding = ImVec2(8, 8);
  style.FramePadding = ImVec2(6, 4);
  style.ItemSpacing = ImVec2(8, 4);
  style.ItemInnerSpacing = ImVec2(4, 4);
  style.IndentSpacing = 16.0f;

  // Borders
  style.WindowBorderSize = 1.0f;
  style.ChildBorderSize = 1.0f;
  style.FrameBorderSize = 0.0f;
  style.TabBorderSize = 0.0f;

  style.ScrollbarSize = 12.0f;
  style.GrabMinSize = 8.0f;

  ImVec4* colors = style.Colors;

  // Background
  colors[ImGuiCol_WindowBg]             = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
  colors[ImGuiCol_ChildBg]              = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
  colors[ImGuiCol_PopupBg]              = ImVec4(0.10f, 0.10f, 0.12f, 0.96f);

  // Borders
  colors[ImGuiCol_Border]               = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
  colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

  // Frame (input fields, checkboxes)
  colors[ImGuiCol_FrameBg]              = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
  colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.24f, 0.24f, 0.30f, 1.00f);
  colors[ImGuiCol_FrameBgActive]        = ImVec4(0.30f, 0.30f, 0.38f, 1.00f);

  // Title bar
  colors[ImGuiCol_TitleBg]              = ImVec4(0.09f, 0.09f, 0.11f, 1.00f);
  colors[ImGuiCol_TitleBgActive]        = ImVec4(0.12f, 0.12f, 0.16f, 1.00f);
  colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.09f, 0.09f, 0.11f, 0.75f);

  // Menu bar
  colors[ImGuiCol_MenuBarBg]            = ImVec4(0.14f, 0.14f, 0.16f, 1.00f);

  // Scrollbar
  colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
  colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.36f, 0.36f, 0.42f, 1.00f);
  colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.44f, 0.44f, 0.52f, 1.00f);

  // Buttons
  colors[ImGuiCol_Button]               = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
  colors[ImGuiCol_ButtonHovered]        = ImVec4(0.32f, 0.32f, 0.42f, 1.00f);
  colors[ImGuiCol_ButtonActive]         = ImVec4(0.38f, 0.38f, 0.50f, 1.00f);

  // Headers (collapsing headers, tree nodes)
  colors[ImGuiCol_Header]               = ImVec4(0.20f, 0.20f, 0.26f, 1.00f);
  colors[ImGuiCol_HeaderHovered]        = ImVec4(0.28f, 0.28f, 0.38f, 1.00f);
  colors[ImGuiCol_HeaderActive]         = ImVec4(0.34f, 0.34f, 0.46f, 1.00f);

  // Separator
  colors[ImGuiCol_Separator]            = ImVec4(0.22f, 0.22f, 0.26f, 1.00f);
  colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.40f, 0.55f, 0.80f, 0.78f);
  colors[ImGuiCol_SeparatorActive]      = ImVec4(0.40f, 0.55f, 0.80f, 1.00f);

  // Resize grip
  colors[ImGuiCol_ResizeGrip]           = ImVec4(0.30f, 0.30f, 0.40f, 0.30f);
  colors[ImGuiCol_ResizeGripHovered]    = ImVec4(0.40f, 0.55f, 0.80f, 0.67f);
  colors[ImGuiCol_ResizeGripActive]     = ImVec4(0.40f, 0.55f, 0.80f, 0.95f);

  // Tabs
  colors[ImGuiCol_Tab]                  = ImVec4(0.14f, 0.14f, 0.18f, 1.00f);
  colors[ImGuiCol_TabHovered]           = ImVec4(0.28f, 0.28f, 0.38f, 1.00f);
  colors[ImGuiCol_TabSelected]          = ImVec4(0.22f, 0.22f, 0.30f, 1.00f);
  colors[ImGuiCol_TabDimmed]            = ImVec4(0.10f, 0.10f, 0.14f, 1.00f);
  colors[ImGuiCol_TabDimmedSelected]    = ImVec4(0.16f, 0.16f, 0.22f, 1.00f);

  // Docking
  colors[ImGuiCol_DockingPreview]       = ImVec4(0.40f, 0.55f, 0.80f, 0.70f);
  colors[ImGuiCol_DockingEmptyBg]       = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);

  // Text
  colors[ImGuiCol_Text]                 = ImVec4(0.88f, 0.88f, 0.90f, 1.00f);
  colors[ImGuiCol_TextDisabled]         = ImVec4(0.46f, 0.46f, 0.50f, 1.00f);

  // Selection / Highlight
  colors[ImGuiCol_CheckMark]            = ImVec4(0.45f, 0.65f, 0.95f, 1.00f);
  colors[ImGuiCol_SliderGrab]           = ImVec4(0.40f, 0.55f, 0.80f, 1.00f);
  colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.50f, 0.65f, 0.90f, 1.00f);

  // Drag drop
  colors[ImGuiCol_DragDropTarget]       = ImVec4(0.45f, 0.65f, 0.95f, 0.90f);

  // Nav
  colors[ImGuiCol_NavHighlight]         = ImVec4(0.40f, 0.55f, 0.80f, 1.00f);
}

void EditorApp::initEditor() {
  editorCamera.setScreenSize(EDITOR_WIDTH, EDITOR_HEIGHT);

  hierarchyPanel = std::make_unique<SceneHierarchyPanel>(*this);
  inspectorPanel = std::make_unique<InspectorPanel>(*this);
  viewportPanel = std::make_unique<ViewportPanel>(*this);
  assetBrowserPanel = std::make_unique<AssetBrowserPanel>(*this);

  // Seed scene with a few test entities
  sauce::Entity cameraEntity("Main Camera");
  cameraEntity.addComponent<TransformComponent>();
  pScene->addEntity(std::move(cameraEntity));

  sauce::Entity lightEntity("Directional Light");
  lightEntity.addComponent<TransformComponent>();
  pScene->addEntity(std::move(lightEntity));

  // Load initial scene file if specified via command line
  if (!initialSceneFile.empty()) {
    openScene(initialSceneFile);
  } else {
    setStatusMessage("SauceEditor ready");
  }
}

void EditorApp::importGLTFToScene(const std::string& path) {
  try {
    size_t before = pScene->getEntities().size();
    pScene->loadGLTFModel(path, true);
    size_t added = pScene->getEntities().size() - before;
    setStatusMessage("Imported " + std::filesystem::path(path).filename().string() +
                     " (" + std::to_string(added) + " entities)");
  } catch (const std::exception& e) {
    setStatusMessage("Import failed: " + std::string(e.what()));
  }
}

void EditorApp::openScene(const std::string& path) {
  try {
    logicalDevice->waitIdle();
    selectionManager.deselect();
    if (pScene->loadFromFile(path)) {
      setStatusMessage("Opened: " + std::filesystem::path(path).filename().string());
    } else {
      setStatusMessage("Failed to open scene: " + path);
    }
  } catch (const std::exception& e) {
    setStatusMessage("Open failed: " + std::string(e.what()));
  }
}

void EditorApp::saveScene() {
  if (pScene->hasFilePath()) {
    saveSceneAs(pScene->getCurrentFilePath());
  } else {
    // Open Save As dialog
    std::string defaultPath = (std::filesystem::current_path() / "assets" / "scene.gltf").string();
    std::strncpy(dialogPathBuf, defaultPath.c_str(), sizeof(dialogPathBuf) - 1);
    dialogPathBuf[sizeof(dialogPathBuf) - 1] = '\0';
    showSaveAsDialog = true;
  }
}

void EditorApp::saveSceneAs(const std::string& path) {
  try {
    if (pScene->saveToFile(path)) {
      setStatusMessage("Saved: " + std::filesystem::path(path).filename().string());
    } else {
      setStatusMessage("Failed to save scene: " + path);
    }
  } catch (const std::exception& e) {
    setStatusMessage("Save failed: " + std::string(e.what()));
  }
}

void EditorApp::uploadMeshGPUResources() {
  for (auto& entity : pScene->getEntitiesMut()) {
    auto mrcs = entity.getComponents<MeshRendererComponent>();
    for (auto* mrc : mrcs) {
      auto mesh = mrc->getMesh();
      if (!mesh || !mesh->isValid()) continue;
      if (!mesh->hasGPUData()) {
        auto& physDev = const_cast<vk::raii::PhysicalDevice&>(*physicalDevice);
        auto& cmdPool = const_cast<vk::raii::CommandPool&>(pRenderer->getCommandPool());
        auto& queue = const_cast<vk::raii::Queue&>(pRenderer->getQueue());
        mesh->initVulkanResources(logicalDevice, physDev, cmdPool, queue);
      }
    }
  }
}

void EditorApp::pickEntityAtScreen(float windowX, float windowY) {
  auto* vp = static_cast<ViewportPanel*>(viewportPanel.get());
  if (!vp) return;

  ImVec2 vpPos = vp->getViewportScreenPos();
  ImVec2 vpSize = vp->getViewportSize();
  if (vpSize.x <= 0 || vpSize.y <= 0) return;

  float localX = windowX - vpPos.x;
  float localY = windowY - vpPos.y;

  // Check if click is within viewport bounds
  if (localX < 0 || localY < 0 || localX > vpSize.x || localY > vpSize.y) return;

  Ray ray = editorCamera.screenToWorldRay(localX, localY, vpSize.x, vpSize.y);

  int bestIdx = -1;
  float bestDist = std::numeric_limits<float>::max();

  auto& entities = pScene->getEntitiesMut();
  for (int i = 0; i < static_cast<int>(entities.size()); ++i) {
    auto& entity = entities[i];
    if (!entity.getActive()) continue;

    auto mrcs = entity.getComponents<MeshRendererComponent>();
    if (mrcs.empty()) continue;

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    auto* tc = entity.getComponent<TransformComponent>();
    if (tc) {
      modelMatrix = tc->getLocalMatrix();
    }

    for (auto* mrc : mrcs) {
      auto mesh = mrc->getMesh();
      if (!mesh || !mesh->isValid()) continue;

      AABB localAABB = AABB::fromVertices(mesh->getVertices());
      AABB worldAABB = localAABB.transformed(modelMatrix);

      float t = 0.0f;
      if (rayIntersectsAABB(ray.origin, ray.direction, worldAABB, t)) {
        if (t < bestDist) {
          bestDist = t;
          bestIdx = i;
        }
      }
    }
  }

  if (bestIdx >= 0) {
    selectionManager.select(bestIdx);
    setStatusMessage("Selected: " + entities[bestIdx].get_name());
  } else {
    selectionManager.deselect();
  }
}

void EditorApp::recordEditorCommandBuffer(vk::raii::CommandBuffer& cmd, uint32_t imageIndex) {
  cmd.begin({});

  // Update UBO with camera matrices - use EditorCamera directly (bypasses Scene Camera)
  float aspect = static_cast<float>(pOffscreenFB->getWidth()) /
                 static_cast<float>(pOffscreenFB->getHeight());
  sauce::UniformBufferObject ubo {
    .model = glm::mat4(1.0f),  // identity for grid
    .view = editorCamera.getViewMatrix(),
    .proj = editorCamera.getProjectionMatrix(aspect),
    .cameraPos = editorCamera.getPosition(),
  };
  ubo.proj[1][1] *= -1;  // Vulkan Y-flip
  memcpy(pRenderer->getCurrentUniformBufferMapped(), &ubo, sizeof(ubo));

  // ========================
  // PASS 1: Render scene to offscreen framebuffer
  // ========================

  // Transition offscreen color image -> ColorAttachmentOptimal
  pRenderer->transitionImageLayout(
    cmd,
    *pOffscreenFB->getColorImage(),
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eColorAttachmentOptimal,
    {},
    vk::AccessFlagBits2::eColorAttachmentWrite,
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::ImageAspectFlagBits::eColor
  );

  // Transition offscreen depth image -> DepthAttachmentOptimal
  pRenderer->transitionImageLayout(
    cmd,
    *pOffscreenFB->getDepthImage(),
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eDepthAttachmentOptimal,
    {},
    vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
    vk::PipelineStageFlagBits2::eTopOfPipe,
    vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
    vk::ImageAspectFlagBits::eDepth
  );

  vk::ClearValue clearColor = vk::ClearColorValue { 0.12f, 0.12f, 0.15f, 1.0f };
  vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

  vk::RenderingAttachmentInfo offscreenColorAttachment {
    .imageView = pOffscreenFB->getColorImageView(),
    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .loadOp = vk::AttachmentLoadOp::eClear,
    .storeOp = vk::AttachmentStoreOp::eStore,
    .clearValue = clearColor,
  };

  vk::RenderingAttachmentInfo offscreenDepthAttachment {
    .imageView = pOffscreenFB->getDepthImageView(),
    .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
    .loadOp = vk::AttachmentLoadOp::eClear,
    .storeOp = vk::AttachmentStoreOp::eStore,
    .clearValue = clearDepth,
  };

  vk::Extent2D offscreenExtent { pOffscreenFB->getWidth(), pOffscreenFB->getHeight() };

  vk::RenderingInfo offscreenRenderingInfo {
    .renderArea = { .offset = { 0, 0 }, .extent = offscreenExtent },
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &offscreenColorAttachment,
    .pDepthAttachment = &offscreenDepthAttachment,
  };

  cmd.beginRendering(offscreenRenderingInfo);

  cmd.setViewport(0, vk::Viewport(0.0f, 0.0f,
    static_cast<float>(offscreenExtent.width),
    static_cast<float>(offscreenExtent.height),
    0.0f, 1.0f));
  cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), offscreenExtent));

  // Draw grid
  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, **pGridPipeline);
  cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
    pGridPipeline->getLayout(), 0, *pRenderer->getCurrentDescriptorSet(), nullptr);
  cmd.draw(6, 1, 0, 0);  // Fullscreen quad (6 vertices, no vertex buffer)

  // Draw scene meshes with active pipeline (unlit or lit based on viewport mode)
  auto* activePipeline = (viewportMode == ViewportMode::Lit) ? pLitPipeline.get() : pUnlitPipeline.get();
  cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, **activePipeline);
  cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
    activePipeline->getLayout(), 0, *pRenderer->getCurrentDescriptorSet(), nullptr);

  for (auto& entity : pScene->getEntitiesMut()) {
    if (!entity.getActive()) continue;

    auto mrcs = entity.getComponents<MeshRendererComponent>();
    if (mrcs.empty()) continue;

    // Get transform for this entity
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    auto* tc = entity.getComponent<TransformComponent>();
    if (tc) {
      modelMatrix = tc->getLocalMatrix();
    }

    for (auto* mrc : mrcs) {
      auto mesh = mrc->getMesh();
      if (!mesh || !mesh->hasGPUData()) continue;

      // Build push constants with material data
      MeshPushConstants pushData {};
      pushData.model = modelMatrix;
      pushData.baseColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f); // default grey
      pushData.metallic = 0.0f;
      pushData.roughness = 0.5f;

      auto material = mrc->getMaterial();
      if (material) {
        auto& props = material->getProperties();
        pushData.baseColor = props.baseColorFactor;
        pushData.metallic = props.metallicFactor;
        pushData.roughness = props.roughnessFactor;
      }

      cmd.pushConstants<MeshPushConstants>(activePipeline->getLayout(),
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, pushData);

      auto& cmdRef = const_cast<vk::raii::CommandBuffer&>(cmd);
      mesh->bind(cmdRef);
      mesh->draw(cmdRef);
    }
  }

  // Draw gizmo for selected entity
  if (selectionManager.hasSelection() && pGizmoRenderer) {
    auto* entity = selectionManager.getSelectedEntity(*pScene);
    if (entity) {
      auto* tc = entity->getComponent<TransformComponent>();
      if (tc) {
        pGizmoRenderer->render(cmd, pRenderer->getCurrentDescriptorSet(),
                               tc->getTranslation(), editorCamera, aspect);
      }
    }
  }

  cmd.endRendering();

  // Transition offscreen color image -> ShaderReadOnlyOptimal for ImGui sampling
  pRenderer->transitionImageLayout(
    cmd,
    *pOffscreenFB->getColorImage(),
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::ImageLayout::eShaderReadOnlyOptimal,
    vk::AccessFlagBits2::eColorAttachmentWrite,
    vk::AccessFlagBits2::eShaderRead,
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::PipelineStageFlagBits2::eFragmentShader,
    vk::ImageAspectFlagBits::eColor
  );

  // ========================
  // PASS 2: Render ImGui to swapchain
  // ========================

  // Transition swapchain image -> ColorAttachmentOptimal
  pRenderer->transitionImageLayout(
    cmd,
    pRenderer->getSwapChain().getImages()[imageIndex],
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eColorAttachmentOptimal,
    {},
    vk::AccessFlagBits2::eColorAttachmentWrite,
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::ImageAspectFlagBits::eColor
  );

  // Transition renderer's depth image for the ImGui pass (ImGui pipeline expects depth format)
  pRenderer->transitionImageLayout(
    cmd,
    *pRenderer->getDepthImage(),
    vk::ImageLayout::eUndefined,
    vk::ImageLayout::eDepthAttachmentOptimal,
    {},
    vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
    vk::PipelineStageFlagBits2::eTopOfPipe,
    vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
    vk::ImageAspectFlagBits::eDepth
  );

  vk::ClearValue swapClearColor = vk::ClearColorValue { 0.0f, 0.0f, 0.0f, 1.0f };
  vk::ClearValue swapClearDepth = vk::ClearDepthStencilValue(1.0f, 0);

  vk::RenderingAttachmentInfo swapColorAttachment {
    .imageView = pRenderer->getSwapChain().getImageViews()[imageIndex],
    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .loadOp = vk::AttachmentLoadOp::eClear,
    .storeOp = vk::AttachmentStoreOp::eStore,
    .clearValue = swapClearColor,
  };

  vk::RenderingAttachmentInfo swapDepthAttachment {
    .imageView = pRenderer->getDepthImageView(),
    .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
    .loadOp = vk::AttachmentLoadOp::eClear,
    .storeOp = vk::AttachmentStoreOp::eDontCare,
    .clearValue = swapClearDepth,
  };

  vk::RenderingInfo swapRenderingInfo {
    .renderArea = {
      .offset = { 0, 0 },
      .extent = pRenderer->getSwapChain().getExtent(),
    },
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &swapColorAttachment,
    .pDepthAttachment = &swapDepthAttachment,
  };

  cmd.beginRendering(swapRenderingInfo);

  // Render ImGui (which includes the viewport panel showing the offscreen texture)
  if (pImGuiRenderer) {
    pImGuiRenderer->render(cmd, imageIndex);
  }

  cmd.endRendering();

  // Transition swapchain image -> PresentSrcKHR
  pRenderer->transitionImageLayout(
    cmd,
    pRenderer->getSwapChain().getImages()[imageIndex],
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::ImageLayout::ePresentSrcKHR,
    vk::AccessFlagBits2::eColorAttachmentWrite,
    {},
    vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    vk::PipelineStageFlagBits2::eBottomOfPipe,
    vk::ImageAspectFlagBits::eColor
  );

  cmd.end();
}

void EditorApp::mainLoop() {
  while (!glfwWindowShouldClose(window)) {
    auto currentFrameTime = std::chrono::steady_clock::now();
    deltaTime = std::chrono::duration<float>(currentFrameTime - lastFrameTime).count();
    lastFrameTime = currentFrameTime;

    glfwPollEvents();
    processInput();

    editorCamera.update(deltaTime);
    editorCamera.syncToSceneCamera(pScene->getCameraRW());

    // Handle viewport resize
    if (viewportPanel) {
      auto* vp = static_cast<ViewportPanel*>(viewportPanel.get());
      if (vp->viewportSizeChanged()) {
        ImVec2 size = vp->getViewportSize();
        uint32_t w = static_cast<uint32_t>(size.x);
        uint32_t h = static_cast<uint32_t>(size.y);
        if (w > 0 && h > 0) {
          logicalDevice->waitIdle();
          pOffscreenFB->resize(w, h);
          editorCamera.setScreenSize(static_cast<float>(w), static_cast<float>(h));
        }
        vp->clearSizeChanged();
      }
    }

    // Upload mesh GPU resources for any newly imported models
    uploadMeshGPUResources();

    pImGuiRenderer->newFrame();
    buildEditorUI();

    pRenderer->drawFrame(logicalDevice, *pScene, pImGuiRenderer.get());
  }

  logicalDevice->waitIdle();
}

void EditorApp::processInput() {
  // Always allow fly mode WASD when in fly mode (cursor is captured)
  if (editorCamera.getMode() == EditorCamera::Mode::Fly) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      editorCamera.flyMoveForward(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      editorCamera.flyMoveBackward(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      editorCamera.flyMoveLeft(deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      editorCamera.flyMoveRight(deltaTime);
    return;
  }

  // In orbit mode, only process input when viewport hovered
  if (!viewportHovered || ImGui::GetIO().WantCaptureKeyboard) {
    return;
  }
}

void EditorApp::buildEditorUI() {
  // Create full-screen dockspace
  ImGuiViewport* vp = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(vp->WorkPos);
  ImGui::SetNextWindowSize(vp->WorkSize);
  ImGui::SetNextWindowViewport(vp->ID);

  ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
  windowFlags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
  windowFlags |= ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

  ImGui::Begin("DockSpace", nullptr, windowFlags);
  ImGui::PopStyleVar(3);

  ImGuiID dockspaceId = ImGui::GetID("EditorDockSpace");

  if (firstFrame) {
    setupDefaultDockLayout(dockspaceId);
    firstFrame = false;
  }

  ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

  // Menu bar
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
        logicalDevice->waitIdle();
        pScene->getEntitiesMut().clear();
        pScene->setCurrentFilePath("");
        selectionManager.deselect();
        setStatusMessage("New scene created");
      }
      if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) {
        std::string defaultPath = pScene->hasFilePath()
          ? pScene->getCurrentFilePath()
          : (std::filesystem::current_path() / "assets" / "scene.gltf").string();
        std::strncpy(dialogPathBuf, defaultPath.c_str(), sizeof(dialogPathBuf) - 1);
        dialogPathBuf[sizeof(dialogPathBuf) - 1] = '\0';
        showOpenDialog = true;
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
        saveScene();
      }
      if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) {
        std::string defaultPath = pScene->hasFilePath()
          ? pScene->getCurrentFilePath()
          : (std::filesystem::current_path() / "assets" / "scene.gltf").string();
        std::strncpy(dialogPathBuf, defaultPath.c_str(), sizeof(dialogPathBuf) - 1);
        dialogPathBuf[sizeof(dialogPathBuf) - 1] = '\0';
        showSaveAsDialog = true;
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Exit", "Esc")) {
        glfwSetWindowShouldClose(window, true);
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Edit")) {
      if (ImGui::MenuItem("Deselect All", "Ctrl+D")) {
        selectionManager.deselect();
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("Hierarchy", nullptr, &showHierarchy);
      ImGui::MenuItem("Inspector", nullptr, &showInspector);
      ImGui::MenuItem("Viewport", nullptr, &showViewport);
      ImGui::MenuItem("Asset Browser", nullptr, &showAssetBrowser);
      ImGui::Separator();
      if (ImGui::MenuItem("Reset Layout")) {
        firstFrame = true; // re-trigger layout on next frame
      }
      ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Entity")) {
      if (ImGui::MenuItem("Add Empty Entity")) {
        sauce::Entity e("New Entity");
        e.addComponent<TransformComponent>();
        pScene->addEntity(std::move(e));
        selectionManager.select(static_cast<int>(pScene->getEntities().size()) - 1);
        setStatusMessage("Created new entity");
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  ImGui::End();

  // Render panels
  if (showHierarchy && hierarchyPanel) {
    hierarchyPanel->render();
  }
  if (showInspector && inspectorPanel) {
    inspectorPanel->render();
  }
  if (showViewport && viewportPanel) {
    viewportPanel->render();
    viewportHovered = static_cast<ViewportPanel*>(viewportPanel.get())->isViewportHovered();
    viewportFocused = static_cast<ViewportPanel*>(viewportPanel.get())->isViewportFocused();
  }
  if (showAssetBrowser && assetBrowserPanel) {
    assetBrowserPanel->render();
  }

  // Status bar at bottom
  {
    if (statusTimer > 0.0f) {
      statusTimer -= deltaTime;
    }

    float statusBarHeight = 24.0f;
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x, vp->WorkPos.y + vp->WorkSize.y - statusBarHeight));
    ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, statusBarHeight));
    ImGuiWindowFlags statusFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoFocusOnAppearing;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 3));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.12f, 1.00f));
    ImGui::Begin("##StatusBar", nullptr, statusFlags);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();

    if (statusTimer > 0.0f && !statusMessage.empty()) {
      ImGui::Text("%s", statusMessage.c_str());
    } else {
      int entityCount = static_cast<int>(pScene->getEntities().size());
      auto& cam = editorCamera;
      glm::vec3 camPos = cam.getPosition();
      ImGui::Text("Entities: %d | Camera: (%.1f, %.1f, %.1f) | %s",
                  entityCount, camPos.x, camPos.y, camPos.z,
                  cam.getMode() == EditorCamera::Mode::Orbit ? "Orbit" : "Fly");
    }

    ImGui::End();
  }

  // Open Scene dialog
  if (showOpenDialog) {
    ImGui::OpenPopup("Open Scene");
    showOpenDialog = false;
  }
  if (ImGui::BeginPopupModal("Open Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("File path:");
    ImGui::SetNextItemWidth(400);
    ImGui::InputText("##openpath", dialogPathBuf, sizeof(dialogPathBuf));
    ImGui::Spacing();
    if (ImGui::Button("Open", ImVec2(120, 0))) {
      openScene(dialogPathBuf);
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }

  // Save Scene As dialog
  if (showSaveAsDialog) {
    ImGui::OpenPopup("Save Scene As");
    showSaveAsDialog = false;
  }
  if (ImGui::BeginPopupModal("Save Scene As", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("File path (.gltf or .glb):");
    ImGui::SetNextItemWidth(400);
    ImGui::InputText("##savepath", dialogPathBuf, sizeof(dialogPathBuf));
    ImGui::Spacing();
    if (ImGui::Button("Save", ImVec2(120, 0))) {
      saveSceneAs(dialogPathBuf);
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void EditorApp::setupDefaultDockLayout(ImGuiID dockspaceId) {
  ImGui::DockBuilderRemoveNode(dockspaceId);
  ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);

  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

  ImGuiID dockLeft, dockCenter;
  ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.20f, &dockLeft, &dockCenter);

  ImGuiID dockRight, dockCenterRemaining;
  ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Right, 0.25f, &dockRight, &dockCenterRemaining);

  ImGuiID dockBottom, dockViewport;
  ImGui::DockBuilderSplitNode(dockCenterRemaining, ImGuiDir_Down, 0.28f, &dockBottom, &dockViewport);

  ImGui::DockBuilderDockWindow("Hierarchy", dockLeft);
  ImGui::DockBuilderDockWindow("Inspector", dockRight);
  ImGui::DockBuilderDockWindow("Asset Browser", dockBottom);
  ImGui::DockBuilderDockWindow("Viewport", dockViewport);

  ImGui::DockBuilderFinish(dockspaceId);
}

void EditorApp::mouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/) {
  auto* app = static_cast<EditorApp*>(glfwGetWindowUserPointer(window));
  if (!app) return;

  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);

  // Only guard PRESS events — releases must always be processed to prevent stuck state
  bool guardPress = false;
  if (action == GLFW_PRESS) {
    bool imguiWants = ImGui::GetIO().WantCaptureMouse;
    bool inFlyMode = app->editorCamera.getMode() == EditorCamera::Mode::Fly;
    guardPress = imguiWants && !app->viewportHovered && !inFlyMode;
  }

  if (button == GLFW_MOUSE_BUTTON_RIGHT) {
    if (action == GLFW_PRESS) {
      if (guardPress) return;
      app->rightMouseDown = true;
      app->lastMouseX = static_cast<float>(xpos);
      app->lastMouseY = static_cast<float>(ypos);
      if (app->viewportHovered) {
        app->editorCamera.beginFlyMode();
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
      }
    } else if (action == GLFW_RELEASE) {
      app->rightMouseDown = false;
      if (app->editorCamera.getMode() == EditorCamera::Mode::Fly) {
        app->editorCamera.endFlyMode();
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      }
    }
  }

  if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
    if (action == GLFW_PRESS) {
      if (guardPress) return;
      app->middleMouseDown = true;
      app->lastMouseX = static_cast<float>(xpos);
      app->lastMouseY = static_cast<float>(ypos);
    } else if (action == GLFW_RELEASE) {
      app->middleMouseDown = false;
    }
  }

  if (button == GLFW_MOUSE_BUTTON_LEFT) {
    if (action == GLFW_PRESS) {
      if (guardPress) return;
      app->leftMouseDown = true;
      app->lastMouseX = static_cast<float>(xpos);
      app->lastMouseY = static_cast<float>(ypos);
      app->mousePressX = static_cast<float>(xpos);
      app->mousePressY = static_cast<float>(ypos);

      // Check gizmo hit first
      if (app->viewportHovered && app->selectionManager.hasSelection() && app->pGizmoRenderer) {
        auto* gizmo = app->pGizmoRenderer->getActiveGizmo();
        auto* entity = app->selectionManager.getSelectedEntity(*app->pScene);
        if (gizmo && entity) {
          auto* tc = entity->getComponent<TransformComponent>();
          if (tc) {
            auto* vp = static_cast<ViewportPanel*>(app->viewportPanel.get());
            ImVec2 vpPos = vp->getViewportScreenPos();
            ImVec2 vpSize = vp->getViewportSize();
            float localX = static_cast<float>(xpos) - vpPos.x;
            float localY = static_cast<float>(ypos) - vpPos.y;
            Ray ray = app->editorCamera.screenToWorldRay(localX, localY, vpSize.x, vpSize.y);

            float dist = glm::length(app->editorCamera.getPosition() - tc->getTranslation());
            float gizmoScale = dist * GizmoRenderer::SCALE_FACTOR;
            GizmoAxis hitAxis = gizmo->hitTest(ray, tc->getTranslation(), gizmoScale);
            if (hitAxis != GizmoAxis::None) {
              gizmo->beginInteraction(hitAxis, ray, tc->getTranslation());
              app->gizmoInteracting = true;
            }
          }
        }
      }
    } else if (action == GLFW_RELEASE) {
      // End gizmo interaction if active
      if (app->gizmoInteracting && app->pGizmoRenderer) {
        auto* gizmo = app->pGizmoRenderer->getActiveGizmo();
        if (gizmo) gizmo->endInteraction();
        app->gizmoInteracting = false;
      } else {
        // Click-to-select: if mouse barely moved, it's a click not a drag
        float dx = static_cast<float>(xpos) - app->mousePressX;
        float dy = static_cast<float>(ypos) - app->mousePressY;
        if (std::sqrt(dx * dx + dy * dy) < 3.0f) {
          app->pickEntityAtScreen(static_cast<float>(xpos), static_cast<float>(ypos));
        }
      }
      app->leftMouseDown = false;
    }
  }
}

void EditorApp::cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
  auto* app = static_cast<EditorApp*>(glfwGetWindowUserPointer(window));
  if (!app) return;

  float xposf = static_cast<float>(xpos);
  float yposf = static_cast<float>(ypos);
  float deltaX = xposf - app->lastMouseX;
  float deltaY = app->lastMouseY - yposf; // Inverted Y

  app->lastMouseX = xposf;
  app->lastMouseY = yposf;

  // Always process fly mode mouse look (cursor is captured)
  if (app->rightMouseDown && app->editorCamera.getMode() == EditorCamera::Mode::Fly) {
    app->editorCamera.flyMouseLook(deltaX, deltaY);
    return;
  }

  // Handle gizmo interaction (takes priority over orbit/pan)
  if (app->gizmoInteracting && app->leftMouseDown && app->pGizmoRenderer) {
    auto* gizmo = app->pGizmoRenderer->getActiveGizmo();
    auto* entity = app->selectionManager.getSelectedEntity(*app->pScene);
    if (gizmo && entity) {
      auto* tc = entity->getComponent<TransformComponent>();
      if (tc) {
        auto* vp = static_cast<ViewportPanel*>(app->viewportPanel.get());
        ImVec2 vpPos = vp->getViewportScreenPos();
        ImVec2 vpSize = vp->getViewportSize();
        float localX = xposf - vpPos.x;
        float localY = yposf - vpPos.y;
        Ray ray = app->editorCamera.screenToWorldRay(localX, localY, vpSize.x, vpSize.y);

        glm::vec3 delta = gizmo->updateInteraction(ray, tc->getTranslation());

        GizmoType type = app->pGizmoRenderer->getActiveGizmoType();
        if (type == GizmoType::Translate) {
          tc->setTranslation(tc->getTranslation() + delta);
        } else if (type == GizmoType::Rotate) {
          // delta encodes angle * axis
          float angle = glm::length(delta);
          if (angle > 1e-6f) {
            glm::vec3 axis = delta / angle;
            glm::quat rot = glm::angleAxis(angle, axis);
            tc->setRotation(rot * tc->getRotation());
          }
        } else if (type == GizmoType::Scale) {
          tc->setScale(tc->getScale() + delta);
        }
      }
    }
    return;
  }

  // Only orbit/pan when viewport is hovered
  if (!app->viewportHovered) return;

  if (app->leftMouseDown && app->editorCamera.getMode() == EditorCamera::Mode::Orbit) {
    app->editorCamera.orbit(deltaX * 0.3f, deltaY * 0.3f);
  } else if (app->middleMouseDown) {
    app->editorCamera.pan(deltaX, deltaY);
  }
}

void EditorApp::scrollCallback(GLFWwindow* window, double /*xoffset*/, double yoffset) {
  auto* app = static_cast<EditorApp*>(glfwGetWindowUserPointer(window));
  if (!app) return;

  if (!app->viewportHovered) return;

  app->editorCamera.zoom(static_cast<float>(yoffset));
}

void EditorApp::keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int mods) {
  auto* app = static_cast<EditorApp*>(glfwGetWindowUserPointer(window));
  if (!app) return;

  bool ctrl = (mods & GLFW_MOD_CONTROL) != 0;
  bool shift = (mods & GLFW_MOD_SHIFT) != 0;

  // Global shortcuts that work even when ImGui wants keyboard
  if (action == GLFW_PRESS && ctrl) {
    if (key == GLFW_KEY_S && shift) {
      // Ctrl+Shift+S = Save As
      std::string defaultPath = app->pScene->hasFilePath()
        ? app->pScene->getCurrentFilePath()
        : (std::filesystem::current_path() / "assets" / "scene.gltf").string();
      std::strncpy(app->dialogPathBuf, defaultPath.c_str(), sizeof(app->dialogPathBuf) - 1);
      app->dialogPathBuf[sizeof(app->dialogPathBuf) - 1] = '\0';
      app->showSaveAsDialog = true;
      return;
    }
    if (key == GLFW_KEY_S) {
      app->saveScene();
      return;
    }
    if (key == GLFW_KEY_O) {
      std::string defaultPath = app->pScene->hasFilePath()
        ? app->pScene->getCurrentFilePath()
        : (std::filesystem::current_path() / "assets" / "scene.gltf").string();
      std::strncpy(app->dialogPathBuf, defaultPath.c_str(), sizeof(app->dialogPathBuf) - 1);
      app->dialogPathBuf[sizeof(app->dialogPathBuf) - 1] = '\0';
      app->showOpenDialog = true;
      return;
    }
    if (key == GLFW_KEY_N) {
      app->logicalDevice->waitIdle();
      app->pScene->getEntitiesMut().clear();
      app->pScene->setCurrentFilePath("");
      app->selectionManager.deselect();
      app->setStatusMessage("New scene created");
      return;
    }
    if (key == GLFW_KEY_D) {
      app->selectionManager.deselect();
      return;
    }
  }

  if (ImGui::GetIO().WantCaptureKeyboard) return;

  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    // If in fly mode, exit fly mode first
    if (app->editorCamera.getMode() == EditorCamera::Mode::Fly) {
      app->editorCamera.endFlyMode();
      app->rightMouseDown = false;
      glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
      return;
    }
    glfwSetWindowShouldClose(window, true);
  }

  if (key == GLFW_KEY_F && action == GLFW_PRESS) {
    auto* entity = app->selectionManager.getSelectedEntity(*app->pScene);
    if (entity) {
      auto* tc = entity->getComponent<TransformComponent>();
      if (tc) {
        app->editorCamera.focusOn(tc->getTranslation());
        app->setStatusMessage("Focused on: " + entity->get_name());
      }
    }
  }

  if (key == GLFW_KEY_DELETE && action == GLFW_PRESS) {
    int idx = app->selectionManager.getSelectedIndex();
    auto& entities = app->pScene->getEntitiesMut();
    if (idx >= 0 && idx < static_cast<int>(entities.size())) {
      std::string name = entities[idx].get_name();
      // Wait for GPU to finish using entity's mesh buffers before destroying
      app->logicalDevice->waitIdle();
      entities.erase(entities.begin() + idx);
      app->selectionManager.deselect();
      app->setStatusMessage("Deleted: " + name);
    }
  }

  // W/E/R for gizmo mode switching (only when not in fly mode and no modifier keys)
  if (action == GLFW_PRESS && app->editorCamera.getMode() != EditorCamera::Mode::Fly && !ctrl) {
    if (key == GLFW_KEY_W) {
      app->activeGizmoMode = GizmoType::Translate;
      if (app->pGizmoRenderer) app->pGizmoRenderer->setActiveGizmo(GizmoType::Translate);
      app->setStatusMessage("Gizmo: Translate");
    }
    if (key == GLFW_KEY_E) {
      app->activeGizmoMode = GizmoType::Rotate;
      if (app->pGizmoRenderer) app->pGizmoRenderer->setActiveGizmo(GizmoType::Rotate);
      app->setStatusMessage("Gizmo: Rotate");
    }
    if (key == GLFW_KEY_R) {
      app->activeGizmoMode = GizmoType::Scale;
      if (app->pGizmoRenderer) app->pGizmoRenderer->setActiveGizmo(GizmoType::Scale);
      app->setStatusMessage("Gizmo: Scale");
    }
  }
}

void EditorApp::dropCallback(GLFWwindow* window, int count, const char** paths) {
  auto* app = static_cast<EditorApp*>(glfwGetWindowUserPointer(window));
  if (!app || !app->assetBrowserPanel) return;

  for (int i = 0; i < count; ++i) {
    app->assetBrowserPanel->handleFileDrop(paths[i]);
  }
}

} // namespace sauce::editor
