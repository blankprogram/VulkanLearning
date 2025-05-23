
// Swapchain.hpp
#pragma once

#include "engine/platform/VulkanDevice.hpp"
#include <vector>
#include <vulkan/vulkan.h>

class Swapchain {
  public:
    Swapchain(VulkanDevice *device, VkSurfaceKHR surface, GLFWwindow *window);
    ~Swapchain();

    void recreate();
    void cleanup();
    VkExtent2D getExtent() const { return extent_; }
    VkSwapchainKHR getSwapchain() const { return swapchain_; }
    const std::vector<VkImageView> &getImageViews() const {
        return imageViews_;
    }
    VkFormat getImageFormat() const { return imageFormat_; }

    const std::vector<VkImage> &getImages() const { return images_; }

  private:
    VulkanDevice *device_;
    VkSurfaceKHR surface_;
    VkExtent2D extent_{};
    VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;
    VkFormat imageFormat_{};
    GLFWwindow *window_;

    void create();
    void createImageViews();
};
