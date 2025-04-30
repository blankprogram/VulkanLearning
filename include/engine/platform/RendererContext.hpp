
#pragma once

#include "engine/platform/DepthResources.hpp"
#include "engine/platform/DescriptorManager.hpp"
#include "engine/platform/FrameSync.hpp"
#include "engine/platform/FramebufferManager.hpp"
#include "engine/platform/RenderPassManager.hpp"
#include "engine/platform/Swapchain.hpp"
#include "engine/platform/UniformManager.hpp"
#include "engine/platform/VulkanDevice.hpp"
#include "engine/render/Camera.hpp"
#include "engine/render/Pipeline.hpp"
#include <GLFW/glfw3.h>
#include <memory>

class RendererContext {
public:
  explicit RendererContext(GLFWwindow *window);
  ~RendererContext();

  void beginFrame();
  void endFrame();
  void recreateSwapchain();
  void cleanup();

  engine::render::Camera &camera() { return cam_; }
  VkCommandBuffer getCurrentCommandBuffer() const {
    return currentCommandBuffer_;
  }
  uint32_t getCurrentImageIndex() const { return currentImageIndex_; }

  VulkanDevice *getDevice() const { return device_.get(); }
  Swapchain *getSwapchain() const { return swapchain_.get(); }

private:
  static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

  void init(GLFWwindow *window);
  void createFramebuffers();

  std::unique_ptr<VulkanDevice> device_;
  std::unique_ptr<Swapchain> swapchain_;
  VmaAllocator allocator_{VK_NULL_HANDLE};

  engine::render::Camera cam_;
  engine::render::Pipeline pipeline_;

  FrameSync frameSync_;
  RenderPassManager renderPassMgr_;
  FramebufferManager framebufferMgr_;
  DepthResources depthResources_;
  UniformManager uniformMgr_;

  VkCommandBuffer commandBuffers_[MAX_FRAMES_IN_FLIGHT]{};
  VkCommandBuffer currentCommandBuffer_{VK_NULL_HANDLE};
  uint32_t currentImageIndex_ = 0;
  uint32_t currentFrame_ = 0;
};
