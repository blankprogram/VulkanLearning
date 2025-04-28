
#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H

static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
#define GLFW_INCLUDE_VULKAN
#include "../render/Pipeline.hpp"
#include "../render/Vertex.hpp"
#include "engine/resources/Texture.hpp"
#include "engine/utils/VulkanHelpers.hpp"
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
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
class VulkanContext {
public:
  VulkanContext(uint32_t w, uint32_t h, const std::string &t);
  ~VulkanContext();
  void Run();

  Texture texture_;

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
  void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

  void createDescriptorSetLayout();
  void createUniformBuffers();
  void createDescriptorPool();
  void createDescriptorSets();
  void updateDescriptorSet(uint32_t frameIndex);
  void updateUniformBuffer(uint32_t frameIndex);

  void createDepthResources();
  void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex,
                           uint32_t frameIndex);

  void recreateSwapchain();
  void cleanupSwapchain();
  void setupDebugMessenger();
  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData);
  bool framebufferResized_ = false;
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

  struct UBO {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
  };

  VkImage depthImage_;
  VkDeviceMemory depthImageMemory_;
  VkImageView depthImageView_;

  VkBuffer indexBuffer_;
  VkDeviceMemory indexBufferMemory_;
  uint32_t indexCount_;
  VkDebugUtilsMessengerEXT debugMessenger_;
  const std::vector<const char *> deviceExtensions_ = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  Pipeline filledPipeline_;
  Pipeline wireframePipeline_;
  bool useWireframe_ = false;
};

#endif // VULKAN_CONTEXT_H
