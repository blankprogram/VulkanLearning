#pragma once
#include "externals/vk_mem_alloc.h"
#include <vulkan/vulkan.h>

class DepthResources {
  public:
    void init(VkDevice device, VmaAllocator allocator, VkPhysicalDevice phys,
              VkExtent2D extent, VkFormat format);
    void cleanup(VkDevice device, VmaAllocator allocator);

    VkImageView view() const { return view_; }
    VkFormat format() const { return format_; }

  private:
    VkImage image_ = VK_NULL_HANDLE;
    VkImageView view_ = VK_NULL_HANDLE;
    VmaAllocation alloc_ = VK_NULL_HANDLE;
    VkFormat format_ = VK_FORMAT_UNDEFINED;
};
