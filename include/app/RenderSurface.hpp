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

struct RenderSurface {

  RenderSurface(const sauce::Instance& instance, GLFWwindow* window) {
    VkSurfaceKHR cSurface;
    if (VkResult res = glfwCreateWindowSurface(**instance, window, nullptr, &cSurface); res != 0) {
      throw std::runtime_error(std::string("Failed to create window surface! Error code: ") + std::to_string(res));
    }
    surface = { *instance, cSurface };
  }

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

