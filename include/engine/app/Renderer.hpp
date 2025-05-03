#pragma once

#include "engine/core/Device.hpp"
#include "engine/core/PhysicalDevice.hpp"
#include "engine/core/Queue.hpp"
#include "engine/core/Surface.hpp"
#include "engine/pipeline/GraphicsPipeline.hpp"
#include "engine/pipeline/RenderPass.hpp"
#include "engine/rendering/CommandBuffer.hpp"
#include "engine/rendering/CommandPool.hpp"
#include "engine/rendering/Fence.hpp"
#include "engine/rendering/Semaphore.hpp"
#include "engine/swapchain/Depthbuffer.hpp"
#include "engine/swapchain/Framebuffer.hpp"
#include "engine/swapchain/Swapchain.hpp"

namespace engine {

class Renderer {
public:
  Renderer(Device &device, PhysicalDevice &physical, Surface &surface,
           vk::Extent2D windowExtent, Queue::FamilyIndices queues);

  ~Renderer() = default;

  void drawFrame();

private:
  void createSwapchain();
  void createDepthBuffer();
  void createFramebuffers();
  void createRenderPass();
  void createGraphicsPipeline();
  void createCommandPoolAndBuffers();
  void createSyncObjects();

  Device &_device;
  PhysicalDevice &_physical;
  Surface &_surface;
  vk::Extent2D _extent;
  Queue::FamilyIndices _queues;

  Swapchain _swapchain;
  DepthBuffer _depth;
  RenderPass _renderPass;
  GraphicsPipeline _pipeline;
  std::vector<Framebuffer> _framebuffers;

  CommandPool _cmdPool;
  std::vector<CommandBuffer> _cmdBuffers;

  std::vector<Semaphore> _imageAvailable;
  std::vector<Semaphore> _renderFinished;
  std::vector<Fence> _inFlightFences;
  size_t _currentFrame = 0;
};

} // namespace engine
