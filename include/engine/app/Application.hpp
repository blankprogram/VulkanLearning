#pragma once

#include "engine/app/Window.hpp"
#include "engine/core/Context.hpp"
#include "engine/core/Instance.hpp"
#include "engine/core/Surface.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class Application {
public:
  Application();
  ~Application();
  void run();

private:
  Context context_;
  Instance instance_;
  Window window_;
  Surface surface_;
};

} // namespace engine
