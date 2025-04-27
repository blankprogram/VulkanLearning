
// VulkanContext.h
#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H
static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
#define GLFW_INCLUDE_VULKAN
#include "Pipeline.h"
#include "Vertex.h"
#include <GLFW/glfw3.h>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

  void createDescriptorSetLayout();
  void createUniformBuffers();
  void createDescriptorPool();
  void createDescriptorSets();
  void updateDescriptorSet(uint32_t frameIndex);
  void updateUniformBuffer(uint32_t frameIndex);
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

  std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores_;
  std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores_;
  std::array<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences_;

  VkDescriptorSetLayout descriptorSetLayout_;
  VkDescriptorPool descriptorPool_;
  std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets_;

  // uniform buffers per frame
  std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> uniformBuffers_;
  std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> uniformBuffersMemory_;

  size_t currentFrame_ = 0;
  VkBuffer vertexBuffer_;
  VkDeviceMemory vertexBufferMemory_;
  uint32_t vertexCount_;
  Pipeline graphicsPipeline_;

  struct UBO {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
  };

  const std::vector<const char *> deviceExtensions_ = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};

#endif // VULKAN_CONTEXT_H
