
#pragma once

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.h>

class RenderResources;
class RenderCommandManager;

class RenderGraph {
public:
  void beginFrame(RenderResources &resources,
                  RenderCommandManager &commandManager, size_t frameIndex,
                  const glm::mat4 &viewProj,
                  VkImage swapchainImage); // ← NEW PARAM

  void endFrame();

  VkCommandBuffer &getCurrentCommandBuffer() { return commandBuffer_; }

private:
  VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
  VkImage swapchainImage_ = VK_NULL_HANDLE;
};
