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

class SauceEngineApp {
public:
  void run() {
    initWindow();
    initVulkan();
    mainLoop();
  }

  ~SauceEngineApp() {
    glfwDestroyWindow(window);
    glfwTerminate();
  }

private:
  GLFWwindow *window;

  std::unique_ptr<sauce::Instance> pInstance;

  std::unique_ptr<sauce::RenderSurface> pRenderSurface;

  sauce::PhysicalDevice physicalDevice = nullptr;
  sauce::LogicalDevice logicalDevice = nullptr;

  std::unique_ptr<sauce::Scene> pScene;

  std::unique_ptr<sauce::Renderer> pRenderer;

  std::unique_ptr<sauce::ImGuiRenderer> pImGuiRenderer;

  std::unique_ptr<sauce::ui::ImGuiComponentManager> pImGuiComponentManager;

  void initVulkan() {
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

    // Initialize UI component system
    pImGuiComponentManager = std::make_unique<sauce::ui::ImGuiComponentManager>();
    pImGuiComponentManager->addComponent(std::make_unique<sauce::ui::HelloWorldWindow>());
    pImGuiComponentManager->addComponent(std::make_unique<sauce::ui::DebugStatsWindow>());
    pImGuiComponentManager->addComponent(std::make_unique<sauce::ui::Text>("textlabel", "Hi I am some text"));
    pImGuiComponentManager->addComponent(std::make_unique<sauce::ui::BulletText>("bullet1", "This is a bullet point"));
    pImGuiComponentManager->addComponent(std::make_unique<sauce::ui::BulletText>("bullet1", "This is another bullet point"));
    pImGuiComponentManager->addComponent(
        std::make_unique<sauce::ui::Button>(
            "myButton",
            "Click Me",
            []() {
                std::cout << "Button clicked!\n";
            }
        )
    );
    static bool myCheck = false;

    pImGuiComponentManager->addComponent(
        std::make_unique<sauce::ui::Checkbox>(
            "myCheckbox",
            "Enable Feature",
            &myCheck,
            [](bool newValue) {
                std::cout << "Checkbox changed: " << newValue << "\n";
            }
        )
    );
    pImGuiComponentManager->addComponent(
    std::make_unique<sauce::ui::LabelText>(
        "fpsLabel",
        "FPS",
        "144"
    )
    );
  static int selectedOption = 0;

  pImGuiComponentManager->addComponent(
      std::make_unique<sauce::ui::RadioButton>(
          "optionA",
          "Option A",
          &selectedOption,
          0,
          [](int v) { std::cout << "Selected: " << v << "\n"; }
      )
  );

  pImGuiComponentManager->addComponent(
      std::make_unique<sauce::ui::RadioButton>(
          "optionB",
          "Option B",
          &selectedOption,
          1,
          [](int v) { std::cout << "Selected: " << v << "\n"; }
      )
  );


  pImGuiComponentManager->addComponent(
      std::make_unique<sauce::ui::TextColored>(
          "warningText",
          "Warning: Low memory!",
          ImVec4(1.0f, 0.2f, 0.2f, 1.0f)
      )
  );

  pImGuiComponentManager->addComponent(
      std::make_unique<sauce::ui::TextWrapped>(
          "longText",
          "This is a long block of text that will automatically wrap inside the ImGui window."
      )
  );

  ImTextureID fontTex = ImGui::GetIO().Fonts->TexID;

  pImGuiComponentManager->addComponent(
      std::make_unique<sauce::ui::Image>(
          "testImage",
          fontTex,
          ImVec2(128, 128)
      )
  );
  pImGuiComponentManager->addComponent(
    std::make_unique<sauce::ui::ImageButton>(
        "testImageButton",
        fontTex,
        ImVec2(64, 64),
        []() {
            std::cout << "ImageButton clicked!\n";
        }
    )
  );




  }

  void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Playground", nullptr, nullptr);
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();

      pImGuiRenderer->newFrame();
      buildExampleUI();

      pRenderer->drawFrame(logicalDevice, *pScene, pImGuiRenderer.get());
    }

    logicalDevice->waitIdle();
  }

  void buildExampleUI() {
    pImGuiComponentManager->renderAll();
  }
};

