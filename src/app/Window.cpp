#include "engine/app/Window.hpp"
#include <stdexcept>

namespace engine {

Window::Window(int width, int height, const char *title)
    : width_(width), height_(height) {

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
  if (!window_)
    throw std::runtime_error("Failed to create GLFW window");
}

Window::~Window() {
  if (window_)
    glfwDestroyWindow(window_);
}

GLFWwindow *Window::get() const { return window_; }

vk::Extent2D Window::getExtent() const {
  // always query actual framebuffer size (handles DPI, minimize, etc)
  int w, h;
  glfwGetFramebufferSize(window_, &w, &h);
  return vk::Extent2D{uint32_t(w), uint32_t(h)};
}

bool Window::shouldClose() const { return glfwWindowShouldClose(window_); }

} // namespace engine
