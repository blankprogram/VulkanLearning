
#include "engine/platform/Swapchain.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>

Swapchain::Swapchain(VulkanDevice *device, VkSurfaceKHR surface,
                     GLFWwindow *window)
    : device_(device), surface_(surface), window_(window) {
  create();
  createImageViews();
}

Swapchain::~Swapchain() { cleanup(); }

void Swapchain::recreate() {
  cleanup();
  create();
  createImageViews();
}

void Swapchain::cleanup() {
  for (auto view : imageViews_) {
    vkDestroyImageView(device_->getDevice(), view, nullptr);
  }
  imageViews_.clear();

  if (swapchain_ != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device_->getDevice(), swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
  }
}

void Swapchain::create() {
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device_->getPhysicalDevice(),
                                            surface_, &capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device_->getPhysicalDevice(), surface_,
                                       &formatCount, nullptr);
  std::vector<VkSurfaceFormatKHR> formats(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(device_->getPhysicalDevice(), surface_,
                                       &formatCount, formats.data());

  VkSurfaceFormatKHR surfaceFormat = formats[0];
  for (const auto &available : formats) {
    if (available.format == VK_FORMAT_B8G8R8A8_UNORM &&
        available.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      surfaceFormat = available;
      break;
    }
  }
  imageFormat_ = surfaceFormat.format;

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(
      device_->getPhysicalDevice(), surface_, &presentModeCount, nullptr);
  std::vector<VkPresentModeKHR> presentModes(presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(device_->getPhysicalDevice(),
                                            surface_, &presentModeCount,
                                            presentModes.data());

  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
  for (const auto &mode : presentModes) {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      presentMode = mode;
      break;
    }
  }

  int width, height;
  glfwGetFramebufferSize(window_, &width, &height);
  VkExtent2D extent = {std::clamp(static_cast<uint32_t>(width),
                                  capabilities.minImageExtent.width,
                                  capabilities.maxImageExtent.width),
                       std::clamp(static_cast<uint32_t>(height),
                                  capabilities.minImageExtent.height,
                                  capabilities.maxImageExtent.height)};

  uint32_t imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 &&
      imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface_;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  createInfo.preTransform = capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  extent_ = createInfo.imageExtent;

  if (vkCreateSwapchainKHR(device_->getDevice(), &createInfo, nullptr,
                           &swapchain_) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create swapchain");
  }

  vkGetSwapchainImagesKHR(device_->getDevice(), swapchain_, &imageCount,
                          nullptr);
  images_.resize(imageCount);
  vkGetSwapchainImagesKHR(device_->getDevice(), swapchain_, &imageCount,
                          images_.data());
}

void Swapchain::createImageViews() {
  imageViews_.resize(images_.size());
  for (size_t i = 0; i < images_.size(); i++) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = images_[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = imageFormat_;
    viewInfo.components = {
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device_->getDevice(), &viewInfo, nullptr,
                          &imageViews_[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create image view");
    }
  }
}
