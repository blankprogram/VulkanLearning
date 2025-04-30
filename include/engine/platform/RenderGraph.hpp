#pragma once

#include <glm/mat4x4.hpp>
#include <vector>
#include <vulkan/vulkan.h>

class RenderResources;
class RenderCommandManager;

class RenderGraph {
public:

void beginFrame(RenderResources &resources,
                RenderCommandManager &commandManager,
                size_t frameIndex,
                uint32_t imageIndex, // <- add this
                const glm::mat4 &viewProj,
                VkImage swapchainImage,
                VkImageLayout currentLayout);


  void endFrame();

void reset() {
  finalLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
}
  VkCommandBuffer &getCurrentCommandBuffer() { return commandBuffer_; }

  // For layout tracking
  VkImageLayout getFinalLayout() const { return finalLayout_; }

private:
  VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
  VkImage swapchainImage_ = VK_NULL_HANDLE;
  VkImageLayout finalLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;
};
