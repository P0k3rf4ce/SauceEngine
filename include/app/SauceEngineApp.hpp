#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <cstdlib>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <app/BufferUtils.hpp>
#include <app/GraphicsPipeline.hpp>
#include <app/Scene.hpp>
#include <app/Instance.hpp>
#include <app/PhysicalDevice.hpp>
#include <app/LogicalDevice.hpp>
#include <app/Renderer.hpp>
#include <app/RenderSurface.hpp>
#include <app/SwapChain.hpp>
#include <app/ImGuiRenderer.hpp>
#include <app/ui/ImGuiComponentManager.hpp>
#include <app/ui/components/HelloWorldWindow.hpp>
#include <app/ui/components/DebugStatsWindow.hpp>
#include <app/ui/components/BulletText.hpp>
#include <app/ui/components/Button.hpp>
#include <app/ui/components/Checkbox.hpp>
#include <app/ui/components/Image.hpp>
#include <app/ui/components/ImageButton.hpp>
#include <app/ui/components/LabelText.hpp>
#include <app/ui/components/RadioButton.hpp>
#include <app/ui/components/Text.hpp>
#include <app/ui/components/TextColored.hpp>
#include <app/ui/components/TextWrapped.hpp>

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

constexpr uint32_t WIDTH = 1280;
constexpr uint32_t HEIGHT = 720;

namespace sauce {

class RigidBodyComponent;

class SauceEngineApp {
public:
  SauceEngineApp(); // Constructor to initialize pImGuiComponentManager
  void run();

  ~SauceEngineApp();

private:
  GLFWwindow *window;

  std::chrono::steady_clock::time_point lastFrameTime = std::chrono::steady_clock::now();
  double deltaFrame = 0.0f;
  double deltaUpdate = 0.0;

  float lastX = 0.0f;
  float lastY = 0.0f;
  bool firstMouse = true;
  bool cursorCaptured = true;
  bool gravePressedLastFrame = false;
  bool demoTriggerPressedLastFrame = false;
  bool cameraCollisionEnabled = false;
  bool dropDemoActive = false;
  bool defaultSceneSpinEnabled = false;
  bool defaultSceneSpinActive = false;
  std::string defaultSceneSpinEntityName;
  glm::vec3 defaultSceneSpinAngularVelocity = glm::vec3(1.35f, 1.9f, 0.65f);
  float cameraCollisionRadius = 0.35f;

  std::unique_ptr<sauce::Instance> pInstance;

  std::unique_ptr<sauce::RenderSurface> pRenderSurface;

  sauce::PhysicalDevice physicalDevice = nullptr;
  sauce::LogicalDevice logicalDevice = nullptr;

  std::unique_ptr<sauce::Renderer> pRenderer;

  std::unique_ptr<sauce::Scene> pScene;
  std::unique_ptr<physics::XPBDSolver> pSolver;

  std::unique_ptr<sauce::ImGuiRenderer> pImGuiRenderer;

  std::unique_ptr<sauce::ui::ImGuiComponentManager> pImGuiComponentManager;
  std::function<void(sauce::ui::ImGuiComponentManager&)> pCustomUIBuilder;

  void initVulkan();
  void initWindow();
  void mainLoop();
  void processInput(float deltaTime);
  static void mouseCallback(GLFWwindow* window, double xposIn, double yposIn);

  void buildExampleUI();

  void uploadMeshGPUResources();
  void setupSceneRenderer();
  void setupXPBDSolver();
  void setupDefaultSceneSpin();
  void updateDefaultSceneSpin(float deltaTime);
  bool isPhysicsDemoScene() const;
  Entity* findDefaultSceneSpinEntity();
  RigidBodyComponent* ensureEntityRigidBody(Entity& entity);
  void configureRigidBodyFromEntity(Entity& entity, RigidBodyComponent& rigidBody);
  void applyCameraCollisionPush(const glm::vec3& previousCameraPosition, float deltaTime);
  void startDropDemo();
  void updateDropDemoForces();
  void syncRigidBodiesToTransforms();
  std::vector<RigidBodyComponent*> collectRigidBodies();
  void recordSceneCommandBuffer(vk::raii::CommandBuffer& cmd, uint32_t imageIndex);

public:
  sauce::ui::ImGuiComponentManager& getImGuiManager() { return *pImGuiComponentManager; }
  void setCustomUIBuilder(std::function<void(sauce::ui::ImGuiComponentManager&)> builder);
  void setSceneFile(const std::string& path) { sceneFile = path; }
  void setCameraCollisionEnabled(bool enabled) { cameraCollisionEnabled = enabled; }

private:
  std::string sceneFile;
};

}
