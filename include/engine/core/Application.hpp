#include "engine/platform/InputManager.hpp"
#include "engine/platform/RendererContext.hpp"
#include "engine/platform/Swapchain.hpp"
#include "engine/platform/VulkanDevice.hpp"
#include "engine/platform/WindowManager.hpp"

#include "engine/utils/ThreadPool.hpp"
#include "engine/world/ChunkManager.hpp"
#include <chrono>
#include <memory>
class Application {
public:
  Application();
  void Run();

private:
  WindowManager windowManager_;
  InputManager inputManager_;
  VulkanDevice vulkanDevice_;
  Swapchain swapchain_;
  RendererContext rendererContext_;
  ChunkManager chunkManager_;
  ThreadPool threadPool_;
  void mainLoop();
};
