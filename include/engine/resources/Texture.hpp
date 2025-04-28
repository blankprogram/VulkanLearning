
#pragma once

#include <string>
#include <vulkan/vulkan.h>

class Texture {
public:
  void Load(VkDevice device, VkPhysicalDevice physicalDevice,
            VkCommandPool commandPool, VkQueue graphicsQueue,
            const std::string &filepath);
  void Cleanup(VkDevice device);

  VkImageView GetImageView() const { return imageView_; }
  VkSampler GetSampler() const { return sampler_; }

private:
  VkImage textureImage_{VK_NULL_HANDLE};
  VkDeviceMemory textureImageMemory_{VK_NULL_HANDLE};
  VkImageView imageView_{VK_NULL_HANDLE};
  VkSampler sampler_{VK_NULL_HANDLE};

  void createImage(VkDevice device, VkPhysicalDevice physicalDevice,
                   uint32_t width, uint32_t height, VkFormat format,
                   VkImageTiling tiling, VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties);
  void createImageView(VkDevice device, VkFormat format);
  void createSampler(VkDevice device);

  void transitionImageLayout(VkDevice device, VkCommandPool commandPool,
                             VkQueue graphicsQueue, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout);
  void copyBufferToImage(VkDevice device, VkCommandPool commandPool,
                         VkQueue graphicsQueue, VkBuffer buffer, uint32_t width,
                         uint32_t height);
};
