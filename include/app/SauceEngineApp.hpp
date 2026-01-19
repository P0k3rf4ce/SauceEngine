#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>

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
      pRenderer->drawFrame(logicalDevice, *pScene);
    }

    logicalDevice->waitIdle();
  }
};

