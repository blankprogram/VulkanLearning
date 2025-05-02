#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class Surface {
  public:
    Surface(const vk::raii::Instance &instance, GLFWwindow *window);
    ~Surface() = default;

    const vk::raii::SurfaceKHR &get() const;

  private:
    vk::raii::SurfaceKHR surface_;
};

} // namespace engine
