#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

namespace engine {

class Window {
  public:
    Window(int width, int height, const char *title);
    ~Window();

    GLFWwindow *getWindow() const;
    vk::Extent2D getExtent() const;

  private:
    GLFWwindow *window_ = nullptr;
    int width_, height_;
};

} // namespace engine
