
#pragma once

#include "engine/platform/CommandBufferRecorder.hpp"
#include "engine/platform/DescriptorManager.hpp"
#include "engine/platform/FrameSync.hpp"
#include "engine/platform/Swapchain.hpp"
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
  engine::render::Camera &camera() { return cam_; }
  VkCommandBuffer getCurrentCommandBuffer() const {
    return currentCommandBuffer_;
  }
  void cleanup();
  uint32_t getCurrentImageIndex() const { return currentImageIndex_; }
  VulkanDevice *getDevice() const { return device_.get(); }
  Swapchain *getSwapchain() const { return swapchain_.get(); }

private:
  static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
  void init(GLFWwindow *window);
  void createRenderPass();
  void createSyncObjects();
  void createUniforms();
  void createFramebuffers();
  std::unique_ptr<VulkanDevice> device_;
  std::unique_ptr<Swapchain> swapchain_;
  FrameSync frameSync_;

  uint32_t currentFrame_ = 0;
  uint32_t currentImageIndex_ = 0;
  VkCommandBuffer currentCommandBuffer_ =
      VK_NULL_HANDLE; // TODO: Hook up proper command buffer

  VkCommandBuffer commandBuffers_[MAX_FRAMES_IN_FLIGHT]{};

  DescriptorManager descriptorMgr_;
  VkBuffer uniformBuffer_{VK_NULL_HANDLE};
  VmaAllocation uniformAllocation_{VK_NULL_HANDLE};
  VmaAllocator allocator_{VK_NULL_HANDLE};
  engine::render::Camera cam_;

  VkRenderPass renderPass_{VK_NULL_HANDLE};
  std::vector<VkFramebuffer> framebuffers_;
  engine::render::Pipeline pipeline_; // your Pipeline struct
  VkExtent2D extent_;                 // current swapchain size
};
