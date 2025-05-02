
#pragma once
#include <array>
#include <vulkan/vulkan.h>
class RenderPassManager {
  public:
    void init(VkDevice device, VkFormat colorFormat, VkFormat depthFormat);
    void cleanup(VkDevice device);
    VkRenderPass get() const { return renderPass_; }

  private:
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
};
