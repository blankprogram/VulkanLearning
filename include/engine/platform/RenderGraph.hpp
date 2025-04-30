#pragma once

#include <glm/mat4x4.hpp>
#include <vector>
#include <vulkan/vulkan.h>

class RenderResources;
class RenderCommandManager;

class RenderGraph {
public:
  void beginFrame(RenderResources &resources,
                  RenderCommandManager &commandManager, size_t frameIndex,
                  const glm::mat4 &viewProj, VkImage swapchainImage,
                  VkImageLayout currentLayout); // NEW PARAM

  void endFrame();

  VkCommandBuffer &getCurrentCommandBuffer() { return commandBuffer_; }

  // For layout tracking
  VkImageLayout getFinalLayout() const { return finalLayout_; }

private:
  VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
  VkImage swapchainImage_ = VK_NULL_HANDLE;
  VkImageLayout finalLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
};
