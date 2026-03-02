#pragma once

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_raii.hpp>

#include <GLFW/glfw3.h>

#include <app/Instance.hpp>

namespace sauce {

constexpr uint32_t WIDTH = 1280;
constexpr uint32_t HEIGHT = 720;

/**
 * Bridge between Vulkan and the window system (GLFW in this case).
 * A surface represents a platform-specific window or display that Vulkan can render to.
 * GLFW handles the platform-specific creation (Win32, X11, Wayland, etc.).
 */
struct RenderSurface {

  /**
   * Creates a Vulkan surface for the given window. Will immediately throw on failure.
   */
  RenderSurface(const sauce::Instance& instance, GLFWwindow* window) {
    VkSurfaceKHR cSurface;
  
    // glfw abstracts away all the platform-specific stuff for us here
    if (VkResult res = glfwCreateWindowSurface(**instance, window, nullptr, &cSurface); res != 0) {
      throw std::runtime_error(std::string("Failed to create window surface! Error code: ") + std::to_string(res));
    }
    surface = { *instance, cSurface }; // to automatically clean up
  }

  /**
   * Access the underlying SurfaceKHR (the Vulkan handle type for surfaces)
   */
  const vk::raii::SurfaceKHR& operator*() const & noexcept {
    return surface;
  }

  const vk::raii::SurfaceKHR* operator->() const & noexcept {
    return &surface;
  }

private:
  vk::raii::SurfaceKHR surface = nullptr;
};

}
