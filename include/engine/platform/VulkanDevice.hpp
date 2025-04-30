
#pragma once

#include "externals/vk_mem_alloc.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

class VulkanDevice {
public:
  explicit VulkanDevice(GLFWwindow *window);
  ~VulkanDevice();

  VulkanDevice(const VulkanDevice &) = delete;
  VulkanDevice &operator=(const VulkanDevice &) = delete;
  VulkanDevice(VulkanDevice &&) = delete;
  VulkanDevice &operator=(VulkanDevice &&) = delete;

  // Accessors
  VkInstance getInstance() const { return instance_; }
  VkDevice getDevice() const { return device_; }
  VkPhysicalDevice getPhysicalDevice() const { return physicalDevice_; }
  VkSurfaceKHR getSurface() const { return surface_; }
  VkQueue getGraphicsQueue() const { return graphicsQueue_; }
  VkCommandPool getCommandPool() const { return commandPool_; }
  VmaAllocator getAllocator() const { return allocator_; }
  uint32_t getGraphicsQueueFamilyIndex() const {
    return graphicsQueueFamilyIndex_;
  }

private:
  // Vulkan handles
  VkInstance instance_{VK_NULL_HANDLE};
  VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
  VkDevice device_{VK_NULL_HANDLE};
  VkSurfaceKHR surface_{VK_NULL_HANDLE};
  VkQueue graphicsQueue_{VK_NULL_HANDLE};
  VkCommandPool commandPool_{VK_NULL_HANDLE};
  VmaAllocator allocator_{VK_NULL_HANDLE};

  uint32_t graphicsQueueFamilyIndex_{};

#ifdef ENABLE_VALIDATION_LAYERS
  VkDebugUtilsMessengerEXT debugMessenger_{VK_NULL_HANDLE};
  void setupDebugMessenger();
  void destroyDebugMessenger();
#endif

  // Helpers
  void createInstance();
  void createSurface(GLFWwindow *window);
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createCommandPool();
  void createAllocator();
};
