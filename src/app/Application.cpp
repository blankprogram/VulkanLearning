#include "engine/app/Application.hpp"
#include "engine/configs/WindowConfig.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace engine {

Application::Application()
    : context_(), instance_(context_.get()),
      window_(windowConfig.width, windowConfig.height, windowConfig.name),
      surface_(instance_.get(), window_.get()) {
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW");
  }
}

Application::~Application() { glfwTerminate(); }

void Application::run() {
  while (!window_.shouldClose()) {
    glfwPollEvents();
  }
}

} // namespace engine
