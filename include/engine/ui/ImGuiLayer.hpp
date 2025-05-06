#pragma once

#include <GLFW/glfw3.h>
#include <glm/ext/vector_float3.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class ImGuiLayer {
public:
  ImGuiLayer(GLFWwindow *window, VkInstance instance, VkDevice device,
             VkPhysicalDevice physicalDevice, uint32_t graphicsQueueFamily,
             VkQueue graphicsQueue, vk::raii::DescriptorPool &descriptorPool,
             VkRenderPass renderPass, uint32_t imageCount);
  ~ImGuiLayer();

  void newFrame();

  void render(VkCommandBuffer cmd);
  void recreate(uint32_t imageCount, VkRenderPass renderPass);
  void setCameraPosition(const glm::vec3 &pos);

private:
  void init();

  GLFWwindow *_window;
  VkInstance _instance;
  VkDevice _device;
  VkPhysicalDevice _physical;
  uint32_t _queueFamily;
  VkQueue _queue;
  vk::raii::DescriptorPool &_descriptorPool;
  VkRenderPass _renderPass;
  uint32_t _imageCount;

  double _lastTime = 0.0;
  int _frameCount = 0;
  glm::vec3 _cameraPos{0.0f, 0.0f, 0.0f};
};

} // namespace engine
