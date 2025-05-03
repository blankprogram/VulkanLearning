
#include "engine/app/Application.hpp"
#include "engine/configs/WindowConfig.hpp"
namespace engine {

Application::Application() {
  initWindow();
  initVulkan();
}

void Application::initWindow() {
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW");
  }

  window_ = std::make_unique<Window>(windowConfig.width, windowConfig.height,
                                     windowConfig.name);
}

void Application::initVulkan() {
  context_ = std::make_unique<Context>();
  instance_ = std::make_unique<Instance>(context_->get());
  surface_ = std::make_unique<Surface>(instance_->get(), window_->get());
}

Application::~Application() { glfwTerminate(); }

void Application::run() {
  while (!window_->shouldClose()) {
    glfwPollEvents();
  }
}

} // namespace engine
