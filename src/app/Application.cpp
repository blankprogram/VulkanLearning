#include "engine/app/Application.hpp"
#include "engine/configs/WindowConfig.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>

namespace engine {

Application::Application() {
  initWindow();
  initVulkan();
  initRenderer();
}

Application::~Application() { glfwTerminate(); }

void Application::initWindow() {
  if (!glfwInit())
    throw std::runtime_error("Failed to initialize GLFW");

  window_ = std::make_unique<Window>(windowConfig.width, windowConfig.height,
                                     windowConfig.name);
}

void Application::initVulkan() {
  context_ = std::make_unique<Context>();
  instance_ = std::make_unique<Instance>(context_->get());

  physicalDevice_ = std::make_unique<PhysicalDevice>(instance_->get());
  surface_ = std::make_unique<Surface>(instance_->get(), window_->get());
  queues_ = Queue::findQueueFamilies(physicalDevice_->get(), surface_->get());
  device_ = std::make_unique<Device>(physicalDevice_->get(), surface_->get());
}

void Application::initRenderer() {
  renderer_ = std::make_unique<Renderer>(*device_, *physicalDevice_, *surface_,
                                         window_->getExtent(), queues_);
}

void Application::run() {
  while (!window_->shouldClose()) {
    renderer_->drawFrame();
    glfwPollEvents();
  }
  device_->get().waitIdle();
}

} // namespace engine
