#include <app/SauceEngineApp.hpp>
#include <app/ui/components/HelloWorldWindow.hpp>
#include <app/ui/components/DebugStatsWindow.hpp>
#include <functional>

namespace sauce {

SauceEngineApp::SauceEngineApp() {
  pImGuiComponentManager = std::make_unique<sauce::ui::ImGuiComponentManager>();
}

SauceEngineApp::~SauceEngineApp() {
    glfwDestroyWindow(window);
    glfwTerminate();
}

void SauceEngineApp::run() {
  initWindow();
  initVulkan(); // ImGui initialized here

  // Call custom UI builder after ImGui is initialized
  if (pCustomUIBuilder) {
    pCustomUIBuilder(*pImGuiComponentManager);
  }

  mainLoop();
}

void SauceEngineApp::setCustomUIBuilder(std::function<void(sauce::ui::ImGuiComponentManager&)> builder) {
  pCustomUIBuilder = std::move(builder);
}

void SauceEngineApp::initVulkan() {
    uint32_t glfwExtensionsCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
    pInstance = std::make_unique<sauce::Instance>(glfwExtensions, glfwExtensionsCount);

    pRenderSurface = std::make_unique<sauce::RenderSurface>(*pInstance, window);

    physicalDevice = { *pInstance };
    logicalDevice = { physicalDevice, *pRenderSurface };

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

    // Initialize ImGui
    sauce::ImGuiRendererCreateInfo imguiCreateInfo{
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
    };

    pImGuiRenderer = std::make_unique<sauce::ImGuiRenderer>(imguiCreateInfo);

    // Add default UI components
    pImGuiComponentManager->addComponent(std::make_unique<sauce::ui::HelloWorldWindow>());
    pImGuiComponentManager->addComponent(std::make_unique<sauce::ui::DebugStatsWindow>());
  }

void SauceEngineApp::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Playground", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

void SauceEngineApp::processInput(float deltaTime) {
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

void SauceEngineApp::mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
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

void SauceEngineApp::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      auto currentFrameTime = std::chrono::steady_clock::now();
      deltaTime = std::chrono::duration<float>(currentFrameTime - lastFrameTime).count();
      lastFrameTime = currentFrameTime;

      glfwPollEvents();
      processInput(deltaTime);

      pImGuiRenderer->newFrame();
      buildExampleUI();

      pRenderer->drawFrame(logicalDevice, *pScene, pImGuiRenderer.get());
    }

    logicalDevice->waitIdle();
  }

void SauceEngineApp::buildExampleUI() {
    pImGuiComponentManager->renderAll();
  }

} // namespace sauce
