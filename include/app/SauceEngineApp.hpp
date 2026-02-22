#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <cstdlib>

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

class SauceEngineApp {
public:
  SauceEngineApp(); // Constructor to initialize pImGuiComponentManager
  void run();

  ~SauceEngineApp();

private:
  GLFWwindow *window;

  std::chrono::steady_clock::time_point lastFrameTime = std::chrono::steady_clock::now();
  float deltaTime = 0.0f;

  float lastX = 0.0f;
  float lastY = 0.0f;
  bool firstMouse = true;

  std::unique_ptr<sauce::Instance> pInstance;

  std::unique_ptr<sauce::RenderSurface> pRenderSurface;

  sauce::PhysicalDevice physicalDevice = nullptr;
  sauce::LogicalDevice logicalDevice = nullptr;

  std::unique_ptr<sauce::Scene> pScene;

  std::unique_ptr<sauce::Renderer> pRenderer;

  std::unique_ptr<sauce::ImGuiRenderer> pImGuiRenderer;

  std::unique_ptr<sauce::ui::ImGuiComponentManager> pImGuiComponentManager;
  std::function<void(sauce::ui::ImGuiComponentManager&)> pCustomUIBuilder;

  void initVulkan();
  void initWindow();
  void mainLoop();

  void buildExampleUI();

public:
  sauce::ui::ImGuiComponentManager& getImGuiManager() { return *pImGuiComponentManager; }
  void setCustomUIBuilder(std::function<void(sauce::ui::ImGuiComponentManager&)> builder);

  void initVulkan() {
    pInstance = std::make_unique<sauce::Instance>();
    pRenderSurface = std::make_unique<sauce::RenderSurface>(*pInstance, window);

    physicalDevice = sauce::PhysicalDevice { *pInstance, *pRenderSurface };
    logicalDevice = sauce::LogicalDevice { physicalDevice };

    sauce::CameraCreateInfo cameraCreateInfo {
      .scrWidth = WIDTH,
      .scrHeight = HEIGHT,
    };

    pScene = std::make_unique<sauce::Scene>( cameraCreateInfo );

    sauce::RendererCreateInfo rendererCreateInfo {
      .physicalDevice = physicalDevice,
      .logicalDevice = logicalDevice,
      .renderSurface = *pRenderSurface,
      .window = window,
    };

    pRenderer = std::make_unique<sauce::Renderer>(rendererCreateInfo);
  }

  void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Playground", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

  void processInput(float deltaTime) {
    if (!pScene) {
      return;
    }

    auto& camera = pScene->getCameraRW();

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      camera.processDirection(Camera::Movement::FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      camera.processDirection(Camera::Movement::BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      camera.processDirection(Camera::Movement::LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      camera.processDirection(Camera::Movement::RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);
  }

  static void mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
    auto* app = static_cast<SauceEngineApp*>(glfwGetWindowUserPointer(window));
    if (!app) {
      return;
    }

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (app->firstMouse) {
      app->lastX = xpos;
      app->lastY = ypos;
      app->firstMouse = false;
    }

    float xoffset = xpos - app->lastX;
    float yoffset = app->lastY - ypos;

    app->lastX = xpos;
    app->lastY = ypos;

    if (!app->pScene) {
      return;
    }

    app->pScene->getCameraRW().processMouseMovement(xoffset, yoffset);
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      auto currentFrameTime = std::chrono::steady_clock::now();
      deltaTime = std::chrono::duration<float>(currentFrameTime - lastFrameTime).count();
      lastFrameTime = currentFrameTime;

      glfwPollEvents();
      processInput(deltaTime);
      pRenderer->drawFrame(logicalDevice, *pScene);
    }

    logicalDevice->waitIdle();
  }
};

}

