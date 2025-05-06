
#pragma once

#include "engine/core/Queue.hpp"
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class Swapchain {
public:
  struct Bundle {
    vk::raii::SwapchainKHR swapchain;
    vk::Format format;
    vk::Extent2D extent;
  };

  // First-time creation
  Swapchain(const vk::raii::PhysicalDevice &physical,
            const vk::raii::Device &device, const vk::raii::SurfaceKHR &surface,
            const vk::Extent2D &windowExtent,
            const Queue::FamilyIndices &indices);

  // Re-creation, passing your old handle so Vulkan can recycle it
  Swapchain(const vk::raii::PhysicalDevice &physical,
            const vk::raii::Device &device, const vk::raii::SurfaceKHR &surface,
            const vk::Extent2D &windowExtent,
            const Queue::FamilyIndices &indices, VkSwapchainKHR oldSwapchain);

  ~Swapchain() = default;
  Swapchain(const Swapchain &) = delete;
  Swapchain &operator=(const Swapchain &) = delete;
  Swapchain(Swapchain &&) = default;
  Swapchain &operator=(Swapchain &&) = default;

  const vk::raii::SwapchainKHR &get() const { return swapchain_; }
  vk::Format imageFormat() const { return format_; }
  vk::Extent2D extent() const { return extent_; }
  const std::vector<vk::Image> &images() const { return images_; }

private:
  // Delegating constructor that takes the fully prepared Bundle
  Swapchain(Bundle &&bundle);

  // Helper builds the Bundle; on first call oldSwapchain will be VK_NULL_HANDLE
  static Bundle createBundle(const vk::raii::PhysicalDevice &physical,
                             const vk::raii::Device &device,
                             const vk::raii::SurfaceKHR &surface,
                             const vk::Extent2D &windowExtent,
                             const Queue::FamilyIndices &indices,
                             VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);

  vk::raii::SwapchainKHR swapchain_;
  std::vector<vk::Image> images_;
  vk::Format format_;
  vk::Extent2D extent_;
};

} // namespace engine
