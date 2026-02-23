#include <iostream>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <cstdlib>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <app/Instance.hpp>
#include <app/PhysicalDevice.hpp>
#include <app/LogicalDevice.hpp>
#include <app/RenderSurface.hpp>
#include <app/Camera.hpp>
#include <app/Renderer.hpp>
#include <app/ModelViewerRenderer.hpp>
#include <app/ImGuiRenderer.hpp>

// Disable validation layers (requires VK_LAYER_KHRONOS_validation to be installed)
constexpr bool enableValidationLayers = false;

constexpr uint32_t WIDTH = 1280;
constexpr uint32_t HEIGHT = 720;

class ModelViewerApp {
public:
  ModelViewerApp(const std::string& modelPath) : modelPath(modelPath) {}

  void run() {
    initWindow();
    initVulkan();
    loadModel();
    mainLoop();
  }

  ~ModelViewerApp() {
    logicalDevice->waitIdle();
    glfwDestroyWindow(window);
    glfwTerminate();
  }

private:
  GLFWwindow* window;
  std::string modelPath;

  std::unique_ptr<sauce::Instance> pInstance;
  std::unique_ptr<sauce::RenderSurface> pRenderSurface;

  sauce::PhysicalDevice physicalDevice = nullptr;
  sauce::LogicalDevice logicalDevice = nullptr;

  std::unique_ptr<sauce::Camera> pCamera;
  std::unique_ptr<sauce::ModelViewerRenderer> pRenderer;
  std::unique_ptr<sauce::ImGuiRenderer> pImGuiRenderer;

  void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Model Viewer", nullptr, nullptr);
  }

  void initVulkan() {
    uint32_t glfwExtensionsCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
    pInstance = std::make_unique<sauce::Instance>(glfwExtensions, glfwExtensionsCount);

    pRenderSurface = std::make_unique<sauce::RenderSurface>(*pInstance, window);

    physicalDevice = { *pInstance };
    logicalDevice = { physicalDevice, *pRenderSurface };

    // Create camera positioned to view the model
    sauce::CameraCreateInfo cameraCreateInfo {
      .scrWidth = static_cast<float>(WIDTH),
      .scrHeight = static_cast<float>(HEIGHT),
      .pos = { 0.0f, 2.0f, 5.0f },
      .fov = 60.0f,
    };
    pCamera = std::make_unique<sauce::Camera>(cameraCreateInfo);

    // Initialize camera to look at origin
    pCamera->lookAt(glm::vec3(0.0f, 2.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Create renderer
    sauce::RendererCreateInfo rendererCreateInfo {
      .physicalDevice = physicalDevice,
      .logicalDevice = logicalDevice,
      .renderSurface = *pRenderSurface,
      .window = window,
    };
    pRenderer = std::make_unique<sauce::ModelViewerRenderer>(rendererCreateInfo);

    // Initialize ImGui
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
  }

  void loadModel() {
    std::cout << "Loading model: " << modelPath << std::endl;
    pRenderer->loadModel(modelPath, physicalDevice, logicalDevice);
  }

  void updateCamera(float time) {
    // Orbital camera: rotate around the origin
    float orbitAngle = time * glm::radians(30.0f); // 30 degrees per second
    float distance = 5.0f;

    float camX = distance * sin(orbitAngle);
    float camZ = distance * cos(orbitAngle);
    float camY = 2.0f;

    glm::vec3 cameraPos(camX, camY, camZ);
    glm::vec3 target(0.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    pCamera->lookAt(cameraPos, target, up);
  }

  void buildUI(float fps) {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(200, 80), ImGuiCond_FirstUseEver);

    ImGui::Begin("Model Viewer Stats");
    ImGui::Text("FPS: %.1f", fps);
    ImGui::Text("Frame Time: %.2f ms", 1000.0f / fps);
    ImGui::End();
  }

  void mainLoop() {
    auto startTime = std::chrono::high_resolution_clock::now();
    auto lastFrameTime = startTime;
    float fps = 0.0f;

    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();

      auto currentTime = std::chrono::high_resolution_clock::now();
      float time = std::chrono::duration<float>(currentTime - startTime).count();

      // Calculate FPS
      float frameDelta = std::chrono::duration<float>(currentTime - lastFrameTime).count();
      if (frameDelta > 0.0f) {
        fps = 1.0f / frameDelta;
      }
      lastFrameTime = currentTime;

      // Update orbital camera
      updateCamera(time);

      // Model rotation: 45 degrees per second
      float modelRotation = time * glm::radians(45.0f);

      // Build UI
      pImGuiRenderer->newFrame();
      buildUI(fps);

      // Render frame
      pRenderer->drawFrame(logicalDevice, *pCamera, modelRotation, pImGuiRenderer.get());
    }

    logicalDevice->waitIdle();
  }
};

int main(int argc, char* argv[]) {
  std::string modelPath = "assets/models/monkey.gltf";

  if (argc > 1) {
    modelPath = argv[1];
  }

  std::cout << "Model Viewer" << std::endl;
  std::cout << "Loading: " << modelPath << std::endl;

  ModelViewerApp app(modelPath);
  try {
    app.run();
  } catch (std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
