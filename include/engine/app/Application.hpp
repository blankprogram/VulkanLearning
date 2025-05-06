#pragma once

#include "engine/app/Renderer.hpp"
#include "engine/app/Window.hpp"
#include "engine/core/Context.hpp"
#include "engine/core/Device.hpp"
#include "engine/core/Instance.hpp"
#include "engine/core/PhysicalDevice.hpp"
#include "engine/core/Queue.hpp"
#include "engine/core/Surface.hpp"

#include <memory>

namespace engine {

class Application {
public:
  Application();
  ~Application();

  void run();
  void onWindowResized(int width, int height);

private:
  void initWindow();
  void initVulkan();
  void initRenderer();

  std::unique_ptr<Window> window_;
  std::unique_ptr<Context> context_;
  std::unique_ptr<Instance> instance_;
  std::unique_ptr<PhysicalDevice> physicalDevice_;
  std::unique_ptr<Device> device_;
  std::unique_ptr<Surface> surface_;
  Queue::FamilyIndices queues_;
  std::unique_ptr<Renderer> renderer_;
};

} // namespace engine
