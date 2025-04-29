
#pragma once

#include "engine/platform/FrameSync.hpp"
#include "engine/platform/Swapchain.hpp"
#include "engine/platform/VulkanDevice.hpp"
#include <GLFW/glfw3.h>
#include <memory>

class RendererContext {
public:
  explicit RendererContext(GLFWwindow *window);
  ~RendererContext();

  void beginFrame();
  void endFrame();
  void recreateSwapchain();

  VkCommandBuffer getCurrentCommandBuffer() const {
    return currentCommandBuffer_;
  }
  void cleanup();
  uint32_t getCurrentImageIndex() const { return currentImageIndex_; }
  VulkanDevice *getDevice() const { return device_.get(); }
  Swapchain *getSwapchain() const { return swapchain_.get(); }

private:
  void init(GLFWwindow *window);
  void createSyncObjects();

  std::unique_ptr<VulkanDevice> device_;
  std::unique_ptr<Swapchain> swapchain_;
  FrameSync frameSync_;

  uint32_t currentFrame_ = 0;
  uint32_t currentImageIndex_ = 0;
  VkCommandBuffer currentCommandBuffer_ =
      VK_NULL_HANDLE; // TODO: Hook up proper command buffer

  static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
};
