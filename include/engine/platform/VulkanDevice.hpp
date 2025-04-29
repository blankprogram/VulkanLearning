#pragma once
#define GLFW_INCLUDE_VULKAN
#include "externals/vk_mem_alloc.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>
class VulkanDevice {
public:
  VulkanDevice(GLFWwindow *window);
  ~VulkanDevice();

  VkInstance instance;
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  VkSurfaceKHR surface;
  VkQueue graphicsQueue;
  VkCommandPool commandPool;

#ifdef ENABLE_VALIDATION_LAYERS
  VkDebugUtilsMessengerEXT debugMessenger;
#endif

private:
  void createInstance();
  void pickPhysicalDevice();
  void createLogicalDevice();
  VmaAllocator allocator;

  uint32_t graphicsQueueFamilyIndex; // Needed to make command pool
};
