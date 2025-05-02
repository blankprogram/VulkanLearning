
#pragma once
#include <vector>
#include <vulkan/vulkan.h>

class FramebufferManager {
  public:
    void init(VkDevice device, VkRenderPass renderPass, VkExtent2D extent,
              const std::vector<VkImageView> &colorViews,
              VkImageView depthView);
    void cleanup(VkDevice device);
    const std::vector<VkFramebuffer> &get() const { return framebuffers_; }

  private:
    std::vector<VkFramebuffer> framebuffers_;
};
