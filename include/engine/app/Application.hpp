#pragma once

#include "engine/app/Window.hpp"
#include "engine/core/Context.hpp"
#include "engine/core/Instance.hpp"
#include "engine/core/Surface.hpp"
#include <GLFW/glfw3.h>
#include <memory>

namespace engine {

class Application {
public:
  Application();
  ~Application();

  void run();

private:
  void initWindow();
  void initVulkan();

  std::unique_ptr<Window> window_;
  std::unique_ptr<Context> context_;
  std::unique_ptr<Instance> instance_;
  std::unique_ptr<Surface> surface_;
};

} // namespace engine
