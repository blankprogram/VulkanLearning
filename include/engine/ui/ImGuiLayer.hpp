#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_raii.hpp>

namespace engine {
namespace ui {

class ImGuiLayer {
public:
  ImGuiLayer(GLFWwindow *window, VkInstance instance, VkDevice device,
             VkPhysicalDevice physicalDevice, uint32_t graphicsQueueFamily,
             VkQueue graphicsQueue, vk::raii::DescriptorPool &descriptorPool,
             VkRenderPass renderPass, uint32_t imageCount);
  ~ImGuiLayer();

  void newFrame();

  void render(VkCommandBuffer cmd);

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
};

} // namespace ui
} // namespace engine
