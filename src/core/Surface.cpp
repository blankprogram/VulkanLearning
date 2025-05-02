#include "engine/core/Surface.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace engine {

static vk::raii::SurfaceKHR createSurface(const vk::raii::Instance &instance, GLFWwindow *window) {
    VkSurfaceKHR rawSurface;
    if (glfwCreateWindowSurface(static_cast<VkInstance>(*instance), window, nullptr, &rawSurface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan surface");
    }
    return vk::raii::SurfaceKHR(instance, rawSurface);
}

Surface::Surface(const vk::raii::Instance &instance, GLFWwindow *window)
    : surface_(createSurface(instance, window)) {}

const vk::raii::SurfaceKHR &Surface::get() const {
    return surface_;
}

} // namespace engine
