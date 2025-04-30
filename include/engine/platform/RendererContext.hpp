

#pragma once

#include "engine/platform/FrameSync.hpp"
#include "engine/platform/RenderCommandManager.hpp"
#include "engine/platform/RenderGraph.hpp"
#include "engine/platform/RenderResources.hpp"
#include "engine/platform/Swapchain.hpp"
#include "engine/platform/VulkanDevice.hpp"
#include "engine/render/Camera.hpp"

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

  VkCommandBuffer &getCurrentCommandBuffer() {
    return renderGraph_.getCurrentCommandBuffer();
  }
  uint32_t getCurrentImageIndex() const { return currentImageIndex_; }

  RenderResources &getRenderResources() { return renderResources_; }
  VulkanDevice *getDevice() const { return device_.get(); }
  Swapchain *getSwapchain() const { return swapchain_.get(); }

  void initImGui(GLFWwindow *window);
  void cleanupImGui();

private:
  static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

  void init(GLFWwindow *window);

  size_t currentFrame_ = 0;
  uint32_t currentImageIndex_ = 0;

  std::unique_ptr<VulkanDevice> device_;
  std::unique_ptr<Swapchain> swapchain_;
  VmaAllocator allocator_{VK_NULL_HANDLE};

  engine::render::Camera cam_;
  FrameSync frameSync_;

  RenderCommandManager commandManager_;
  RenderGraph renderGraph_;
  RenderResources renderResources_;

  VkDescriptorPool imguiDescriptorPool_{VK_NULL_HANDLE}; // ✅ Added
};
