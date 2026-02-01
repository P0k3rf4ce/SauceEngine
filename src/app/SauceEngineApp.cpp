#include <app/SauceEngineApp.hpp>
#include <app/ui/components/HelloWorldWindow.hpp>
#include <app/ui/components/DebugStatsWindow.hpp>
#include <functional>

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
  }

void SauceEngineApp::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();

      pImGuiRenderer->newFrame();
      buildExampleUI();

      pRenderer->drawFrame(logicalDevice, *pScene, pImGuiRenderer.get());
    }

    logicalDevice->waitIdle();
  }

void SauceEngineApp::buildExampleUI() {
    pImGuiComponentManager->renderAll();
  }
