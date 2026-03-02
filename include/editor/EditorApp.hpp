#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <app/Instance.hpp>
#include <app/PhysicalDevice.hpp>
#include <app/LogicalDevice.hpp>
#include <app/RenderSurface.hpp>
#include <app/Renderer.hpp>
#include <app/Scene.hpp>
#include <app/ImGuiRenderer.hpp>

#include <editor/SelectionManager.hpp>
#include <editor/EditorCamera.hpp>
#include <editor/OffscreenFramebuffer.hpp>
#include <editor/gizmos/Gizmo.hpp>

#include <memory>
#include <chrono>
#include <filesystem>

namespace sauce {
class MeshRendererComponent;
} // namespace sauce

namespace sauce::editor {

struct MeshPushConstants {
  glm::mat4 model;       // 64 bytes
  glm::vec4 baseColor;   // 16 bytes
  float metallic;        // 4 bytes
  float roughness;       // 4 bytes
  float pad[2];          // 8 bytes alignment
};
// Total: 96 bytes (within 128-byte minimum guarantee)

enum class ViewportMode { Unlit, Lit };

class EditorPanel;
class SceneHierarchyPanel;
class InspectorPanel;
class ViewportPanel;
class AssetBrowserPanel;
class GizmoRenderer;

class EditorApp {
public:
  EditorApp();
  ~EditorApp();

  void run();

  sauce::Scene& getScene() { return *pScene; }
  const sauce::Scene& getScene() const { return *pScene; }
  SelectionManager& getSelectionManager() { return selectionManager; }
  EditorCamera& getEditorCamera() { return editorCamera; }
  GLFWwindow* getWindow() { return window; }
  float getDeltaTime() const { return deltaTime; }

  void createEmptyEntity();
  void createBoxEntity();
  void createBallEntity();
  
  const sauce::PhysicalDevice& getPhysicalDevice() const { return physicalDevice; }
  const sauce::LogicalDevice& getLogicalDevice() const { return logicalDevice; }
  sauce::Renderer& getRenderer() { return *pRenderer; }

  OffscreenFramebuffer* getOffscreenFramebuffer() { return pOffscreenFB.get(); }

  ViewportMode getViewportMode() const { return viewportMode; }
  void setViewportMode(ViewportMode mode) { viewportMode = mode; }

  void setStatusMessage(const std::string& msg) { statusMessage = msg; statusTimer = 5.0f; }

  void importGLTFToScene(const std::string& path);
  void replaceModelOnComponent(MeshRendererComponent& mrc, const std::string& path);
  void clearModelOnComponent(MeshRendererComponent& mrc);

  void openScene(const std::string& path);
  void saveScene();
  void saveSceneAs(const std::string& path);

  void setInitialSceneFile(const std::string& path) { initialSceneFile = path; }

private:
  void initWindow();
  void initVulkan();
  void initEditor();
  void setupEditorTheme();
  void mainLoop();
  void buildEditorUI();
  void setupDefaultDockLayout(ImGuiID dockspaceId);
  void processInput();

  void recordEditorCommandBuffer(vk::raii::CommandBuffer& cmd, uint32_t imageIndex);
  void uploadMeshGPUResources();
  void pickEntityAtScreen(float windowX, float windowY);

  static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
  static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
  static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
  static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
  static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
  static void dropCallback(GLFWwindow* window, int count, const char** paths);

  GLFWwindow* window = nullptr;

  std::chrono::steady_clock::time_point lastFrameTime = std::chrono::steady_clock::now();
  float deltaTime = 0.0f;

  float lastMouseX = 0.0f;
  float lastMouseY = 0.0f;
  float mousePressX = 0.0f;
  float mousePressY = 0.0f;
  bool rightMouseDown = false;
  bool middleMouseDown = false;
  bool leftMouseDown = false;

  std::unique_ptr<sauce::Instance> pInstance;
  std::unique_ptr<sauce::RenderSurface> pRenderSurface;
  sauce::PhysicalDevice physicalDevice = nullptr;
  sauce::LogicalDevice logicalDevice = nullptr;

  std::unique_ptr<sauce::Scene> pScene;
  std::unique_ptr<sauce::Renderer> pRenderer;
  std::unique_ptr<sauce::ImGuiRenderer> pImGuiRenderer;

  // Editor rendering resources
  std::unique_ptr<OffscreenFramebuffer> pOffscreenFB;
  std::unique_ptr<sauce::GraphicsPipeline> pGridPipeline;
  std::unique_ptr<sauce::GraphicsPipeline> pUnlitPipeline;
  std::unique_ptr<sauce::GraphicsPipeline> pLitPipeline;
  std::unique_ptr<GizmoRenderer> pGizmoRenderer;

  SelectionManager selectionManager;
  EditorCamera editorCamera;

  std::unique_ptr<SceneHierarchyPanel> hierarchyPanel;
  std::unique_ptr<InspectorPanel> inspectorPanel;
  std::unique_ptr<ViewportPanel> viewportPanel;
  std::unique_ptr<AssetBrowserPanel> assetBrowserPanel;

  bool showHierarchy = true;
  bool showInspector = true;
  bool showViewport = true;
  bool showAssetBrowser = true;

  bool firstFrame = true;
  bool viewportHovered = false;
  bool viewportFocused = false;

  ViewportMode viewportMode = ViewportMode::Unlit;
  GizmoType activeGizmoMode = GizmoType::Translate;
  bool gizmoInteracting = false;

  std::string statusMessage;
  float statusTimer = 0.0f;

  // File dialog state
  bool showOpenDialog = false;
  bool showSaveAsDialog = false;
  char dialogPathBuf[512] = {};

  std::string initialSceneFile;
};

} // namespace sauce::editor
