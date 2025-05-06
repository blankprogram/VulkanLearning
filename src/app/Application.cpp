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

Application::~Application() {
  renderer_.reset();
  device_.reset();
  surface_.reset();
  instance_.reset();
  context_.reset();
  window_.reset();
  glfwTerminate();
}

void Application::initWindow() {
  if (!glfwInit())
    throw std::runtime_error("Failed to initialize GLFW");

  window_ = std::make_unique<Window>(windowConfig.width, windowConfig.height,
                                     windowConfig.name);

  GLFWwindow *w = window_->get();
  glfwSetWindowUserPointer(w, this);
  glfwSetFramebufferSizeCallback(w, [](GLFWwindow *win, int newW, int newH) {
    auto app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(win));
    app->onWindowResized(newW, newH);
  });

  glfwSetKeyCallback(w, [](GLFWwindow *win, int key, int sc, int action,
                           int m) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
      return;
    auto app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(win));
    const float step = 0.2f;
    glm::vec3 delta{0.0f};
    if (key == GLFW_KEY_W)
      delta.z -= step;
    if (key == GLFW_KEY_S)
      delta.z += step;
    if (key == GLFW_KEY_A)
      delta.x -= step;
    if (key == GLFW_KEY_D)
      delta.x += step;
    if (key == GLFW_KEY_Q)
      delta.y -= step;
    if (key == GLFW_KEY_E)
      delta.y += step;
    app->camera().processKeyboard(delta);
  });

  // mouse look
  static bool firstMouse = true;
  static double lastX = 0, lastY = 0;
  glfwSetCursorPosCallback(w, [](GLFWwindow *win, double xpos, double ypos) {
    auto app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(win));
    if (firstMouse) {
      lastX = xpos;
      lastY = ypos;
      firstMouse = false;
    }
    float dx = float(xpos - lastX);
    float dy = float(lastY - ypos);
    lastX = xpos;
    lastY = ypos;
    app->camera().processMouse(dx, dy);
  });
}

void Application::onWindowResized(int width, int height) {
  if (renderer_) {
    renderer_->onWindowResized(width, height);
  }
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

  renderer_ = std::make_unique<Renderer>(
      *device_, *physicalDevice_, *surface_, window_->getExtent(), queues_,
      window_->get(), static_cast<VkInstance>(*instance_->get()),
      camera_ // ← pass camera
  );
}

void Application::run() {
  while (!window_->shouldClose()) {
    renderer_->drawFrame();
    glfwPollEvents();
  }
  device_->get().waitIdle();
}

} // namespace engine
