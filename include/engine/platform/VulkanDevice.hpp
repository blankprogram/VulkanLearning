#pragma once
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

private:
  void createInstance();
  void pickPhysicalDevice();
  void createLogicalDevice();
};
