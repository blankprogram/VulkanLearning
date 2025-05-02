#include "engine/platform/DepthResources.hpp"
#include <stdexcept>

void DepthResources::init(VkDevice device, VmaAllocator allocator,
                          VkPhysicalDevice phys, VkExtent2D extent,
                          VkFormat format) {
    format_ = format;
    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = {extent.width, extent.height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    if (vmaCreateImage(allocator, &imageInfo, &allocInfo, &image_, &alloc_,
                       nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create depth image");
    }

    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = image_;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &view_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create depth image view");
    }
}

void DepthResources::cleanup(VkDevice device, VmaAllocator allocator) {
    if (view_)
        vkDestroyImageView(device, view_, nullptr);
    if (image_)
        vmaDestroyImage(allocator, image_, alloc_);
    view_ = VK_NULL_HANDLE;
    image_ = VK_NULL_HANDLE;
    alloc_ = VK_NULL_HANDLE;
}
