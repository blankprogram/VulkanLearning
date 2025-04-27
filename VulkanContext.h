
// VulkanContext.h
#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H

#define GLFW_INCLUDE_VULKAN
#include "Pipeline.h"
#include "Vertex.h"
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <cstring>
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

  void createVertexBuffer();
  void createBuffer(VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags,
                    VkBuffer &, VkDeviceMemory &);
  uint32_t findMemoryType(uint32_t, VkMemoryPropertyFlags) const;
  void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

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

  VkBuffer vertexBuffer_;
  VkDeviceMemory vertexBufferMemory_;
  uint32_t vertexCount_;
  Pipeline graphicsPipeline_;

  const std::vector<const char *> deviceExtensions_ = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};

#endif // VULKAN_CONTEXT_H
