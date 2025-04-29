
// Swapchain.hpp
#pragma once

#include "engine/platform/VulkanDevice.hpp"
#include <vector>
#include <vulkan/vulkan.h>

class Swapchain {
public:
  Swapchain(VulkanDevice *device, VkSurfaceKHR surface);
  ~Swapchain();

  void recreate();
  void cleanup();

  VkSwapchainKHR getSwapchain() const { return swapchain_; }
  const std::vector<VkImageView> &getImageViews() const { return imageViews_; }
  VkFormat getImageFormat() const { return imageFormat_; }

private:
  VulkanDevice *device_;
  VkSurfaceKHR surface_;

  VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
  std::vector<VkImage> images_;
  std::vector<VkImageView> imageViews_;
  VkFormat imageFormat_{};

  void create();
  void createImageViews();
};
