#include <app/SauceEngineApp.hpp>
#include <app/ui/components/HelloWorldWindow.hpp>
#include <app/ui/components/DebugStatsWindow.hpp>
#include <app/ui/components/SettingsWindow.hpp>
#include <functional>
#include <filesystem>

namespace sauce {

SauceEngineApp::SauceEngineApp() {
  pImGuiComponentManager = std::make_unique<sauce::ui::ImGuiComponentManager>();

  Log::init();
  settingsManager.load();
  Log::setPalantirMode(settingsManager.get().palantirMode);
  SAUCE_LOG("App", "SauceEngine starting up");
  SAUCE_LOG_VERBOSE("Settings", "Palantir Mode enabled -- verbose logging active");

  settingsManager.setOnChangeCallback([this](const EditorSettings& s) {
    applySettings(s);
  });
}

SauceEngineApp::~SauceEngineApp() {
    SAUCE_LOG("App", "SauceEngine shutting down");
    Log::shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
}

void SauceEngineApp::run() {
  initWindow();
  initVulkan();

  // Call custom UI builder after ImGui is initialized
  if (pCustomUIBuilder) {
    pCustomUIBuilder(*pImGuiComponentManager);
  }

  applySettings(settingsManager.get());
  mainLoop();
}

void SauceEngineApp::setCustomUIBuilder(std::function<void(sauce::ui::ImGuiComponentManager&)> builder) {
  pCustomUIBuilder = std::move(builder);
}

void SauceEngineApp::initVulkan() {
    uint32_t glfwExtensionsCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
    pInstance = std::make_unique<sauce::Instance>(glfwExtensions, glfwExtensionsCount);
    SAUCE_LOG_VERBOSE("Vulkan", "Instance created");

    pRenderSurface = std::make_unique<sauce::RenderSurface>(*pInstance, window);

    physicalDevice = { *pInstance };
    logicalDevice = { physicalDevice, *pRenderSurface };

    const auto& s = settingsManager.get();
    sauce::CameraCreateInfo cameraCreateInfo {
      .scrWidth = WIDTH,
      .scrHeight = HEIGHT,
      .fov = s.fieldOfView,
      .movementSpeed = s.cameraSpeed,
      .mouseSensitivity = s.mouseSensitivity,
    };

    pScene = std::make_unique<sauce::Scene>( cameraCreateInfo );

    sauce::RendererCreateInfo rendererCreateInfo {
      .physicalDevice = physicalDevice,
      .logicalDevice = logicalDevice,
      .renderSurface = *pRenderSurface,
      .window = window,
      .vsync = settingsManager.get().vsync,
    };

    pRenderer = std::make_unique<sauce::Renderer>(rendererCreateInfo);
    SAUCE_LOG_VERBOSE("Vulkan", "Renderer created (vsync: {})", settingsManager.get().vsync ? "on" : "off");

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
      .depthFormat = sauce::GraphicsPipeline::findDepthFormat(physicalDevice),
    };

    pImGuiRenderer = std::make_unique<sauce::ImGuiRenderer>(imguiCreateInfo);
    SAUCE_LOG("ImGui", "ImGui initialized");

    ImGui::GetIO().FontGlobalScale = settingsManager.get().imguiScale;

    // Add default UI components
    pImGuiComponentManager->addComponent(std::make_unique<sauce::ui::HelloWorldWindow>());
    pImGuiComponentManager->addComponent(std::make_unique<sauce::ui::DebugStatsWindow>());
    pImGuiComponentManager->addComponent(std::make_unique<sauce::ui::SettingsWindow>(settingsManager));
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
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
      glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS && !gravePressedLastFrame) {
      cursorCaptured = !cursorCaptured;
      glfwSetInputMode(window, GLFW_CURSOR, cursorCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
      firstMouse = true;
      SAUCE_LOG_VERBOSE("Input", "Cursor {}", cursorCaptured ? "captured" : "released");
    }
    gravePressedLastFrame = glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS;

    if (!cursorCaptured || !pScene) {
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
  }

void SauceEngineApp::mouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
    auto* app = static_cast<SauceEngineApp*>(glfwGetWindowUserPointer(window));
    if (!app || !app->cursorCaptured) {
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

void SauceEngineApp::applySettings(const sauce::EditorSettings& s) {
    ImGui::GetIO().FontGlobalScale = s.imguiScale;

    Log::setPalantirMode(s.palantirMode);

    pImGuiComponentManager->setComponentEnabled("DebugStatsWindow", s.showDebugStats);

    if (pScene) {
      auto& camera = pScene->getCameraRW();
      camera.setMouseSensitivity(s.mouseSensitivity);
      camera.setMovementSpeed(s.cameraSpeed);
      camera.setFOV(s.fieldOfView);
    }

    if (!s.workingDirectory.empty()) {
      std::error_code ec;
      std::filesystem::current_path(s.workingDirectory, ec);
      if (ec) {
        SAUCE_LOG("Settings", "Failed to set working directory to '{}': {}", s.workingDirectory, ec.message());
      } else {
        SAUCE_LOG_VERBOSE("Settings", "Working directory set to '{}'", std::filesystem::current_path().string());
      }
    }

    SAUCE_LOG_VERBOSE("Settings", "Settings applied (scale={:.2f}, sensitivity={:.2f}, speed={:.1f}, fov={:.0f})",
        s.imguiScale, s.mouseSensitivity, s.cameraSpeed, s.fieldOfView);
  }

} // namespace sauce
