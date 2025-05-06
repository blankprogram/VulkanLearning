#pragma once

#include "engine/core/Device.hpp"
#include "engine/core/PhysicalDevice.hpp"
#include "engine/core/Queue.hpp"
#include "engine/core/Surface.hpp"
#include "engine/memory/Buffer.hpp"
#include "engine/pipeline/GraphicsPipeline.hpp"
#include "engine/pipeline/PipelineLayout.hpp"
#include "engine/pipeline/RenderPass.hpp"
#include "engine/rendering/CommandBuffer.hpp"
#include "engine/rendering/CommandPool.hpp"
#include "engine/rendering/Fence.hpp"
#include "engine/rendering/Semaphore.hpp"
#include "engine/swapchain/Depthbuffer.hpp"
#include "engine/swapchain/Framebuffer.hpp"
#include "engine/swapchain/ImageView.hpp"
#include "engine/swapchain/Swapchain.hpp"
#include "engine/ui/ImGuiLayer.hpp"

#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class Renderer {
public:
  Renderer(Device &device, PhysicalDevice &physical, Surface &surface,
           vk::Extent2D windowExtent, Queue::FamilyIndices queues,
           GLFWwindow *window, VkInstance rawInstance);

  ~Renderer() = default;
  void drawFrame();
  void onWindowResized(int newWidth, int newHeight);

private:
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);
  // setup helpers
  void createSwapchain();
  void createDepthBuffer();
  void createFramebuffers();
  void createRenderPass();
  void createGraphicsPipeline();
  void createCommandPoolAndBuffers();
  void createSyncObjects();
  void createCubeResources();
  void recordCommandBuffers();
  void recreateSwapchain();
  void updateUniformBuffer(uint32_t currentImage);

  void cleanupSwapchain();
  void createSwapchainResources();

  struct Vertex {
    glm::vec3 pos, color;
  };

  Device &_device;
  PhysicalDevice &_physical;
  Surface &_surface;
  vk::Extent2D _extent;
  bool _framebufferResized = false;
  Queue::FamilyIndices _queues;

  Swapchain _swapchain;
  DepthBuffer _depth;
  RenderPass _renderPass;
  std::unique_ptr<PipelineLayout> _pipelineLayout;
  std::unique_ptr<GraphicsPipeline> _pipeline;
  std::vector<Framebuffer> _framebuffers;

  CommandPool _cmdPool;
  std::vector<CommandBuffer> _cmdBuffers;

  std::vector<Semaphore> _imageAvailable;
  std::vector<Semaphore> _renderFinished;
  std::vector<Fence> _inFlightFences;
  std::vector<VkFence> _imagesInFlight;
  size_t _currentFrame = 0;

  // cube mesh & UBO
  std::vector<Vertex> _vertices;
  std::vector<uint16_t> _indices;
  std::unique_ptr<Buffer> _vertexBuffer;
  std::unique_ptr<Buffer> _indexBuffer;
  std::vector<std::unique_ptr<Buffer>> _uniformBuffers;

  std::unique_ptr<vk::raii::DescriptorSetLayout> _uboSetLayout;
  std::unique_ptr<vk::raii::DescriptorPool> _descriptorPool;
  std::vector<VkDescriptorSet> _descriptorSets;

  GLFWwindow *_window;
  VkInstance _rawInstance;
  std::unique_ptr<ImGuiLayer> imguiLayer_;
  std::vector<ImageView> _colorImageViews;
};

} // namespace engine
