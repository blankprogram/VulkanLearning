
#include "engine/app/Renderer.hpp"
#include "engine/configs/PipelineConfig.hpp"
#include "engine/pipeline/ShaderModule.hpp"
#include "engine/swapchain/ImageView.hpp"
#include "engine/utils/UniformBufferObject.hpp"
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>

using namespace std::chrono;

namespace engine {

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

void Renderer::drawFrame() {
  static auto startTime = steady_clock::now();

  // 1) wait + reset fence
  VkFence currentFence = _inFlightFences[_currentFrame].get();
  vkWaitForFences(*_device.get(), 1, &currentFence, VK_TRUE, UINT64_MAX);
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

  if (_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
    vkWaitForFences(*_device.get(), 1, &_imagesInFlight[imageIndex], VK_TRUE,
                    UINT64_MAX);
  }
  _imagesInFlight[imageIndex] = currentFence;

  // Update UBO
  float time = duration<float>(steady_clock::now() - startTime).count();
  UniformBufferObject ubo{};
  ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f),
                          glm::vec3(0, 0, 1));
  ubo.view =
      glm::lookAt(glm::vec3(2, 2, 2), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
  ubo.proj = glm::perspective(
      glm::radians(45.0f), _extent.width / (float)_extent.height, 0.1f, 10.0f);
  ubo.proj[1][1] *= -1;

  void *data = _uniformBuffers[_currentFrame].map();
  memcpy(data, &ubo, sizeof(ubo));
  _uniformBuffers[_currentFrame].unmap();

  // 3) submit
  VkSemaphore waitSemaphores[] = {_imageAvailable[_currentFrame].get()};
  VkSemaphore signalSemaphores[] = {_renderFinished[_currentFrame].get()};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  VkCommandBuffer rawCmd = _cmdBuffers[imageIndex].get();
  submitInfo.pCommandBuffers = &rawCmd;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  vkQueueSubmit(*_device.graphicsQueue(), 1, &submitInfo, currentFence);

  // 4) present
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  VkSwapchainKHR sc = *_swapchain.get();
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &sc;
  presentInfo.pImageIndices = &imageIndex;

  VkResult presRes = vkQueuePresentKHR(*_device.presentQueue(), &presentInfo);
  if (presRes == VK_ERROR_OUT_OF_DATE_KHR || presRes == VK_SUBOPTIMAL_KHR) {
    recreateSwapchain();
  } else if (presRes != VK_SUCCESS) {
    throw std::runtime_error("Failed to present swapchain image");
  }

  // 5) next frame
  _currentFrame = (_currentFrame + 1) % _inFlightFences.size();
}

void Renderer::createCubeResources() {
  // Define 24 vertices (6 faces × 4 verts) with distinct face-colours
  _vertices = {
      // front (red)
      {{-1, -1, 1}, {1, 0, 0}},
      {{1, -1, 1}, {1, 0, 0}},
      {{1, 1, 1}, {1, 0, 0}},
      {{-1, 1, 1}, {1, 0, 0}},
      // right (green)
      {{1, -1, 1}, {0, 1, 0}},
      {{1, -1, -1}, {0, 1, 0}},
      {{1, 1, -1}, {0, 1, 0}},
      {{1, 1, 1}, {0, 1, 0}},
      // back (blue)
      {{1, -1, -1}, {0, 0, 1}},
      {{-1, -1, -1}, {0, 0, 1}},
      {{-1, 1, -1}, {0, 0, 1}},
      {{1, 1, -1}, {0, 0, 1}},
      // left (yellow)
      {{-1, -1, -1}, {1, 1, 0}},
      {{-1, -1, 1}, {1, 1, 0}},
      {{-1, 1, 1}, {1, 1, 0}},
      {{-1, 1, -1}, {1, 1, 0}},
      // top (cyan)
      {{-1, 1, 1}, {0, 1, 1}},
      {{1, 1, 1}, {0, 1, 1}},
      {{1, 1, -1}, {0, 1, 1}},
      {{-1, 1, -1}, {0, 1, 1}},
      // bottom (magenta)
      {{-1, -1, -1}, {1, 0, 1}},
      {{1, -1, -1}, {1, 0, 1}},
      {{1, -1, 1}, {1, 0, 1}},
      {{-1, -1, 1}, {1, 0, 1}},
  };

  _indices = {
      0,  1,  2,  2,  3,  0,  // front
      4,  5,  6,  6,  7,  4,  // right
      8,  9,  10, 10, 11, 8,  // back
      12, 13, 14, 14, 15, 12, // left
      16, 17, 18, 18, 19, 16, // top
      20, 21, 22, 22, 23, 20  // bottom
  };

  // Create staging → device-local vertex & index buffers (omitting
  // command‐buffer copy code)
  vk::DeviceSize vbSize = sizeof(Vertex) * _vertices.size();
  vk::DeviceSize ibSize = sizeof(uint16_t) * _indices.size();

  // Vertex
  Buffer stagingVB(_physical.get(), _device.get(), vbSize,
                   vk::BufferUsageFlagBits::eTransferSrc,
                   vk::MemoryPropertyFlagBits::eHostVisible |
                       vk::MemoryPropertyFlagBits::eHostCoherent);
  stagingVB.copyFrom(_vertices.data(), vbSize);

  _vertexBuffer = Buffer(_physical.get(), _device.get(), vbSize,
                         vk::BufferUsageFlagBits::eVertexBuffer |
                             vk::BufferUsageFlagBits::eTransferDst,
                         vk::MemoryPropertyFlagBits::eDeviceLocal);
  // ... submit copy from stagingVB → _vertexBuffer via one‐time command buffer

  // Index
  Buffer stagingIB(_physical.get(), _device.get(), ibSize,
                   vk::BufferUsageFlagBits::eTransferSrc,
                   vk::MemoryPropertyFlagBits::eHostVisible |
                       vk::MemoryPropertyFlagBits::eHostCoherent);
  stagingIB.copyFrom(_indices.data(), ibSize);

  _indexBuffer = Buffer(_physical.get(), _device.get(), ibSize,
                        vk::BufferUsageFlagBits::eIndexBuffer |
                            vk::BufferUsageFlagBits::eTransferDst,
                        vk::MemoryPropertyFlagBits::eDeviceLocal);
  // ... submit copy from stagingIB → _indexBuffer

  // Create Uniform buffers, descriptor‐set layout & pool
  {
    vk::DescriptorSetLayoutBinding uboBinding{};
    uboBinding.binding = 0;
    uboBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
    uboBinding.descriptorCount = 1;
    uboBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

    _uboSetLayout = vk::raii::DescriptorSetLayout(
        _device.get(),
        vk::DescriptorSetLayoutCreateInfo{}.setBindingCount(1).setPBindings(
            &uboBinding));

    std::vector<vk::DescriptorPoolSize> poolSizes = {
        {vk::DescriptorPoolSize{}
             .setType(vk::DescriptorType::eUniformBuffer)
             .setDescriptorCount(MAX_FRAMES_IN_FLIGHT)}};

    _descriptorPool = vk::raii::DescriptorPool(
        _device.get(),
        vk::DescriptorPoolCreateInfo{}
            .setMaxSets(MAX_FRAMES_IN_FLIGHT)
            .setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
            .setPPoolSizes(poolSizes.data()));

    // Allocate
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                                 *_uboSetLayout);
    _descriptorSets = _descriptorPool.allocate(
        _device.get(), vk::DescriptorSetAllocateInfo{}
                           .setDescriptorPool(*_descriptorPool)
                           .setDescriptorSetCount(MAX_FRAMES_IN_FLIGHT)
                           .setPSetLayouts(layouts.data()));

    // Write
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
      vk::DescriptorBufferInfo bufferInfo{_uniformBuffers[i].get(), 0,
                                          sizeof(UniformBufferObject)};
      vk::WriteDescriptorSet write{};
      write.dstSet = _descriptorSets[i].get();
      write.dstBinding = 0;
      write.descriptorCount = 1;
      write.descriptorType = vk::DescriptorType::eUniformBuffer;
      write.pBufferInfo = &bufferInfo;
      _device.get().updateDescriptorSets(1, &write, 0, nullptr);
    }
  }
}

void Renderer::createSwapchain() { /*…*/ }
void Renderer::createDepthBuffer() { /*…*/ }
void Renderer::createFramebuffers() { /*…*/ }
void Renderer::createRenderPass() { /*…*/ }
void Renderer::createGraphicsPipeline() { /*…*/ }
void Renderer::createCommandPoolAndBuffers() { /*…*/ }
void Renderer::createSyncObjects() { /*…*/ }
void Renderer::recordCommandBuffers() {
  for (size_t i = 0; i < _cmdBuffers.size(); ++i) {
    auto cmd = _cmdBuffers[i].get();
    // … begin passes, clear, etc.

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipeline->get());
    VkBuffer vbs[] = {_vertexBuffer.get()};
    VkDeviceSize off = 0;
    cmd.bindVertexBuffers(0, 1, vbs, &off);
    cmd.bindIndexBuffer(_indexBuffer.get(), 0, vk::IndexType::eUint16);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                           *_pipelineLayout.get(), 0, 1,
                           &_descriptorSets[i].get(), 0, nullptr);
    cmd.drawIndexed(static_cast<uint32_t>(_indices.size()), 1, 0, 0, 0);

    // … end pass, end cmd
  }
}

void Renderer::recreateSwapchain() { /*…*/ }

} // namespace engine
