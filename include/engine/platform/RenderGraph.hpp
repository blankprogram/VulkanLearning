

#pragma once

#include <glm/mat4x4.hpp>
#include <vulkan/vulkan.h>

class RenderResources;
class RenderCommandManager;

class RenderGraph {
public:
  void beginFrame(RenderResources &resources,
                  RenderCommandManager &commandManager, size_t frameIndex,
                  const glm::mat4 &viewProj);
  void endFrame();

  VkCommandBuffer getCurrentCommandBuffer() const { return commandBuffer_; }

private:
  VkCommandBuffer commandBuffer_ = VK_NULL_HANDLE;
};
