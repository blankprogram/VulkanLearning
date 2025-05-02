#include "engine/app/Window.hpp"
#include <stdexcept>

namespace engine {

Window::Window(int width, int height, const char *title)
    : width_(width), height_(height) {
    if (!glfwInit())
        throw std::runtime_error("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_)
        throw std::runtime_error("Failed to create GLFW window");
}

Window::~Window() {
    if (window_)
        glfwDestroyWindow(window_);
    glfwTerminate();
}

GLFWwindow *Window::get() const { return window_; }

vk::Extent2D Window::getExtent() const {
    return {static_cast<uint32_t>(width_), static_cast<uint32_t>(height_)};
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

} // namespace engine
