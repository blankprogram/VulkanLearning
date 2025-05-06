#include "engine/app/Renderer.hpp"
#include "engine/configs/PipelineConfig.hpp"
#include "engine/pipeline/ShaderModule.hpp"

#include <chrono>
#include <cstring> // memcpy
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <vulkan/vulkan_raii.hpp>

namespace engine {
using namespace std::chrono;

Renderer::Renderer(Device &device, PhysicalDevice &physical, Surface &surface,
                   vk::Extent2D windowExtent, Queue::FamilyIndices queues)
    : _device(device), _physical(physical), _surface(surface),
      _extent(windowExtent), _queues(queues),
      _swapchain(physical.get(), device.get(), surface.get(), windowExtent,
                 queues),
      _depth(physical.get(), device.get(), windowExtent),
      _renderPass(device.get(), _swapchain.imageFormat(), _depth.getFormat()),
      _cmdPool(device.get(), queues.graphics.value()) {
  createCubeResources();
  createDepthBuffer();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  createCommandPoolAndBuffers();
  createSyncObjects();
  recordCommandBuffers();
}

void Renderer::recreateSwapchain() {
  _device.get().waitIdle();
  createSwapchain();
  createDepthBuffer();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  createCommandPoolAndBuffers();
  recordCommandBuffers();
}

void Renderer::drawFrame() {
  // 1) wait + reset
  VkFence fence = _inFlightFences[_currentFrame].get();
  vkWaitForFences(*_device.get(), 1, &fence, VK_TRUE, UINT64_MAX);
  _inFlightFences[_currentFrame].reset();

  // 2) acquire
  uint32_t imageIndex;
  VkResult r = vkAcquireNextImageKHR(
      *_device.get(), *_swapchain.get(), UINT64_MAX,
      _imageAvailable[_currentFrame].get(), VK_NULL_HANDLE, &imageIndex);

  if (r == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapchain();
    return;
  } else if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("Failed to acquire swapchain image");
  }

  // 2a) if already in flight, wait
  if (_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
    vkWaitForFences(*_device.get(), 1, &_imagesInFlight[imageIndex], VK_TRUE,
                    UINT64_MAX);
  }
  _imagesInFlight[imageIndex] = fence;

  // 3) submit
  VkSemaphore waitSems[] = {_imageAvailable[_currentFrame].get()};
  VkSemaphore signalSems[] = {_renderFinished[_currentFrame].get()};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSems;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  {
    VkCommandBuffer cb = _cmdBuffers[imageIndex].get();
    submitInfo.pCommandBuffers = &cb;
  }
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSems;

  vkQueueSubmit(*_device.graphicsQueue(), 1, &submitInfo, fence);

  // 4) present
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSems;
  {
    VkSwapchainKHR sc = *_swapchain.get();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &sc;
  }
  presentInfo.pImageIndices = &imageIndex;

  VkResult pres = vkQueuePresentKHR(*_device.presentQueue(), &presentInfo);
  if (pres == VK_ERROR_OUT_OF_DATE_KHR || pres == VK_SUBOPTIMAL_KHR) {
    recreateSwapchain();
  } else if (pres != VK_SUCCESS) {
    throw std::runtime_error("Failed to present swapchain image");
  }

  // 5) next frame
  _currentFrame = (_currentFrame + 1) % _inFlightFences.size();
}

void Renderer::createCubeResources() {
  // --- fill _vertices and _indices with your cube data ---
  // e.g. _vertices = { ... }; _indices = { ... };

  vk::DeviceSize vbSize = sizeof(Vertex) * _vertices.size();
  vk::DeviceSize ibSize = sizeof(uint16_t) * _indices.size();
  vk::DeviceSize uboSize = sizeof(UniformBufferObject);

  // staging
  Buffer stagingVB(_physical.get(), _device.get(), vbSize,
                   vk::BufferUsageFlagBits::eTransferSrc,
                   vk::MemoryPropertyFlagBits::eHostVisible |
                       vk::MemoryPropertyFlagBits::eHostCoherent);
  stagingVB.copyFrom(_vertices.data(), vbSize);

  Buffer stagingIB(_physical.get(), _device.get(), ibSize,
                   vk::BufferUsageFlagBits::eTransferSrc,
                   vk::MemoryPropertyFlagBits::eHostVisible |
                       vk::MemoryPropertyFlagBits::eHostCoherent);
  stagingIB.copyFrom(_indices.data(), ibSize);

  // device-local
  _vertexBuffer =
      std::make_unique<Buffer>(_physical.get(), _device.get(), vbSize,
                               vk::BufferUsageFlagBits::eVertexBuffer |
                                   vk::BufferUsageFlagBits::eTransferDst,
                               vk::MemoryPropertyFlagBits::eDeviceLocal);
  _indexBuffer =
      std::make_unique<Buffer>(_physical.get(), _device.get(), ibSize,
                               vk::BufferUsageFlagBits::eIndexBuffer |
                                   vk::BufferUsageFlagBits::eTransferDst,
                               vk::MemoryPropertyFlagBits::eDeviceLocal);

  // (submit your one-time copy commands staging→device-local here)

  // uniform buffers
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    _uniformBuffers[i] =
        std::make_unique<Buffer>(_physical.get(), _device.get(), uboSize,
                                 vk::BufferUsageFlagBits::eUniformBuffer,
                                 vk::MemoryPropertyFlagBits::eHostVisible |
                                     vk::MemoryPropertyFlagBits::eHostCoherent);
  }

  // descriptor set layout
  vk::DescriptorSetLayoutBinding uboB{};
  uboB.binding = 0;
  uboB.descriptorType = vk::DescriptorType::eUniformBuffer;
  uboB.descriptorCount = 1;
  uboB.stageFlags = vk::ShaderStageFlagBits::eVertex;

  _uboSetLayout = std::make_unique<vk::raii::DescriptorSetLayout>(
      _device.get(),
      vk::DescriptorSetLayoutCreateInfo{}.setBindingCount(1).setPBindings(
          &uboB));

  // pool
  vk::DescriptorPoolSize poolSize{};
  poolSize.type = vk::DescriptorType::eUniformBuffer;
  poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

  _descriptorPool = std::make_unique<vk::raii::DescriptorPool>(
      _device.get(), vk::DescriptorPoolCreateInfo{}
                         .setMaxSets(MAX_FRAMES_IN_FLIGHT)
                         .setPoolSizeCount(1)
                         .setPPoolSizes(&poolSize));

  // allocate
  _descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  {
    std::vector<VkDescriptorSetLayout> layouts(
        MAX_FRAMES_IN_FLIGHT,
        static_cast<VkDescriptorSetLayout>(*_uboSetLayout->operator*()));
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool =
        static_cast<VkDescriptorPool>(*_descriptorPool->operator*());
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    allocInfo.pSetLayouts = layouts.data();

    VkDevice vkDev = static_cast<VkDevice>(*_device.get());
    if (vkAllocateDescriptorSets(vkDev, &allocInfo, _descriptorSets.data()) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to allocate descriptor sets");
    }
  }

  // write
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = static_cast<VkBuffer>(_uniformBuffers[i]->get());
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet w{};
    w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    w.dstSet = _descriptorSets[i];
    w.dstBinding = 0;
    w.dstArrayElement = 0;
    w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w.descriptorCount = 1;
    w.pBufferInfo = &bufferInfo;

    VkDevice vkDev = static_cast<VkDevice>(*_device.get());
    vkUpdateDescriptorSets(vkDev, 1, &w, 0, nullptr);
  }
}

void Renderer::createFramebuffers() {
  _colorImageViews.clear();
  auto const &images = _swapchain.images();
  _colorImageViews.reserve(images.size());
  for (auto image : images) {
    _colorImageViews.emplace_back(_device.get(), image,
                                  _swapchain.imageFormat(),
                                  vk::ImageAspectFlagBits::eColor, 1, 1);
  }

  _framebuffers.clear();
  _framebuffers.reserve(images.size());
  for (size_t i = 0; i < images.size(); ++i) {
    std::array<vk::ImageView, 2> atts = {*(_colorImageViews[i].get()),
                                         *(_depth.getView())};
    _framebuffers.emplace_back(
        _device.get(), _renderPass.get(), _extent,
        std::vector<vk::ImageView>{atts.begin(), atts.end()});
  }
}

void Renderer::createSyncObjects() {
  size_t n = _framebuffers.size();
  _imageAvailable.clear();
  _renderFinished.clear();
  _inFlightFences.clear();
  _imagesInFlight.assign(n, VK_NULL_HANDLE);

  for (size_t i = 0; i < n; ++i) {
    _imageAvailable.emplace_back(_device.get());
    _renderFinished.emplace_back(_device.get());
    _inFlightFences.emplace_back(_device.get(), true);
  }
}

void Renderer::createSwapchain() {
  _swapchain = Swapchain(_physical.get(), _device.get(), _surface.get(),
                         _extent, _queues);
}

void Renderer::createDepthBuffer() {
  _depth = DepthBuffer(_physical.get(), _device.get(), _extent);
}

void Renderer::createRenderPass() {
  _renderPass =
      RenderPass(_device.get(), _swapchain.imageFormat(), _depth.getFormat());
}

void Renderer::createGraphicsPipeline() {
  auto cfg = defaultPipelineConfig(_extent);

  ShaderModule vert{_device.get(), "shaders/triangle.vert.spv"};
  ShaderModule frag{_device.get(), "shaders/triangle.frag.spv"};
  std::array<vk::PipelineShaderStageCreateInfo, 2> stages = {
      vert.stageInfo(vk::ShaderStageFlagBits::eVertex),
      frag.stageInfo(vk::ShaderStageFlagBits::eFragment)};

  _pipelineLayout = std::make_unique<PipelineLayout>(
      _device.get(), cfg.setLayouts, cfg.pushConstants);

  _pipeline = std::make_unique<GraphicsPipeline>(
      _device.get(), *_pipelineLayout, _renderPass.get(), cfg.vertexInput,
      cfg.inputAssembly, cfg.rasterizer, cfg.multisampling, cfg.depthStencil,
      cfg.colorBlend,
      std::vector<vk::PipelineShaderStageCreateInfo>{stages.begin(),
                                                     stages.end()},
      std::vector<vk::PipelineViewportStateCreateInfo>{cfg.viewportState},
      cfg.dynamicState,
      GraphicsPipeline::Config{cfg.viewportExtent, cfg.msaaSamples});
}

void Renderer::recordCommandBuffers() {
  for (size_t i = 0; i < _cmdBuffers.size(); ++i) {
    auto cmd = _cmdBuffers[i].get();

    // ... vkBeginCommandBuffer + vkCmdBeginRenderPass omitted for brevity ...

    // bind pipeline
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipeline->get());

    // bind vertex
    VkBuffer vbs[] = {static_cast<VkBuffer>(_vertexBuffer->get())};
    VkDeviceSize offs[] = {0};
    cmd.bindVertexBuffers(0, vbs, offs);

    // bind index
    cmd.bindIndexBuffer(static_cast<VkBuffer>(_indexBuffer->get()), 0,
                        vk::IndexType::eUint16);

    // bind descriptor sets
    {
      VkCommandBuffer raw = static_cast<VkCommandBuffer>(cmd);
      vkCmdBindDescriptorSets(
          raw, VK_PIPELINE_BIND_POINT_GRAPHICS,
          static_cast<VkPipelineLayout>(*(_pipelineLayout->get().operator*())),
          0, static_cast<uint32_t>(_descriptorSets.size()),
          _descriptorSets.data(), 0, nullptr);
    }

    // draw
    vkCmdDrawIndexed(static_cast<VkCommandBuffer>(cmd),
                     static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);

    // ... vkCmdEndRenderPass + vkEndCommandBuffer omitted ...
  }
}

void Renderer::createCommandPoolAndBuffers() {
  _cmdBuffers.clear();
  _cmdBuffers.reserve(_framebuffers.size());
  for (size_t i = 0; i < _framebuffers.size(); ++i) {
    _cmdBuffers.emplace_back(_device.get(), _cmdPool.get());
  }
}

} // namespace engine
