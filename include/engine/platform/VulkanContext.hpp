
#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H

#define GLFW_INCLUDE_VULKAN
#include "../render/Pipeline.hpp"
#include "../render/Vertex.hpp"
#include "engine/render/Renderer.hpp"
#include "engine/resources/Texture.hpp"
#include "engine/utils/VulkanHelpers.hpp"
#include "thirdparty/entt.hpp"
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

#include "engine/resources/Model.hpp"

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
  void onResize(int w, int h);
  void createGlobalDescriptorSetLayout();
  void createMaterialDescriptorSetLayout();
  void createDescriptorPool(); // unchanged
  void createGlobalDescriptorSets();
  void updateDescriptorSet(uint32_t frameIndex);
  void updateUniformBuffer(uint32_t frameIndex);
  void updateTextureDescriptor();
  void createDepthResources();
  void recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex,
                           uint32_t frameIndex);
  void createUniformBuffers();
  void recreateSwapchain(uint32_t width, uint32_t height);
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

  // set 0: global UBO + texture
  VkDescriptorSetLayout globalDescriptorSetLayout_;
  // set 1: per‐material sampler only
  VkDescriptorSetLayout materialDescriptorSetLayout_;
  VkDescriptorPool descriptorPool_;
  std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> descriptorSets_;

  // uniform buffers per frame
  std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> uniformBuffers_;
  std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> uniformBuffersMemory_;

  void onMouseMoved(float xpos, float ypos);
  void processInput(float dt);

  struct UBO {
    glm::mat4 view;
    glm::mat4 proj;
  };

  VkImage depthImage_;
  VkDeviceMemory depthImageMemory_;
  VkImageView depthImageView_;

  VkDebugUtilsMessengerEXT debugMessenger_;
  const std::vector<const char *> deviceExtensions_ = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  Pipeline filledPipeline_;
  Pipeline wireframePipeline_;
  bool useWireframe_ = false;

  int newWidth_ = 0;
  int newHeight_ = 0;

  float lastX_ = width_ / 2.0f, lastY_ = height_ / 2.0f;
  bool firstMouse_ = true;

  float cameraAspect_;
  entt::registry registry_;
  std::unique_ptr<Renderer> renderer_;
  int currentFrame_;
  std::vector<std::unique_ptr<Mesh>> meshes_;
  std::vector<std::unique_ptr<Material>> materials_;

  std::vector<std::unique_ptr<Model>> models_;
};

#endif // VULKAN_CONTEXT_H
