
// VulkanContext.h
#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H

#define GLFW_INCLUDE_VULKAN
#include "Pipeline.h"
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

class VulkanContext {
public:
  VulkanContext(uint32_t w, uint32_t h, const std::string &t);
  ~VulkanContext();
  void Run();

private:
  void initWindow();
  void initVulkan();
  void mainLoop();
  void cleanup();

  void createInstance();
  void pickPhysicalDevice();
  uint32_t findGraphicsQueueFamily();
  void createLogicalDevice(uint32_t gfxFam);
  void createSwapchain(uint32_t gfxFam);
  void createImageViews();
  void createRenderPass();
  void createFramebuffers();
  void createCommandPool(uint32_t gfxFam);
  void createCommandBuffers();
  void createSyncObjects();
  void drawFrame();

  GLFWwindow *window_;
  uint32_t width_, height_;
  std::string title_;

  VkInstance instance_;
  VkSurfaceKHR surface_;
  VkPhysicalDevice physicalDevice_{VK_NULL_HANDLE};
  VkDevice device_;
  VkQueue graphicsQueue_;

  VkSwapchainKHR swapchain_;
  VkFormat swapchainImageFormat_;
  VkExtent2D swapchainExtent_;
  std::vector<VkImage> swapchainImages_;
  std::vector<VkImageView> swapchainImageViews_;
  std::vector<VkFramebuffer> swapchainFramebuffers_;

  VkRenderPass renderPass_;
  VkCommandPool commandPool_;
  std::vector<VkCommandBuffer> commandBuffers_;

  VkSemaphore imageAvailableSemaphore_;
  VkSemaphore renderFinishedSemaphore_;

  Pipeline graphicsPipeline_;

  const std::vector<const char *> deviceExtensions_ = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};

#endif // VULKAN_CONTEXT_H
