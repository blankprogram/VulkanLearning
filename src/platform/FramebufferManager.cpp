#include "engine/platform/FramebufferManager.hpp"
#include <stdexcept>

void FramebufferManager::init(VkDevice device, VkRenderPass renderPass,
                              VkExtent2D extent,
                              const std::vector<VkImageView> &colorViews,
                              VkImageView depthView) {
  framebuffers_.resize(colorViews.size());

  for (size_t i = 0; i < colorViews.size(); ++i) {
    VkImageView attachments[] = {colorViews[i], depthView};

    VkFramebufferCreateInfo createInfo{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    createInfo.renderPass = renderPass;
    createInfo.attachmentCount = 2;
    createInfo.pAttachments = attachments;
    createInfo.width = extent.width;
    createInfo.height = extent.height;
    createInfo.layers = 1;

    if (vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffers_[i]) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to create framebuffer");
    }
  }
}

void FramebufferManager::cleanup(VkDevice device) {
  for (auto fb : framebuffers_) {
    if (fb != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(device, fb, nullptr);
    }
  }
  framebuffers_.clear();
}
