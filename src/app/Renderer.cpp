
#include "engine/app/Renderer.hpp"
#include "engine/configs/PipelineConfig.hpp"
#include "engine/pipeline/ShaderModule.hpp"
#include "engine/utils/UniformBufferObject.hpp"

#include <chrono>
#include <cstring> // memcpy
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include <vulkan/vulkan_raii.hpp>
namespace engine {

Renderer::Renderer(Device &device, PhysicalDevice &physical, Surface &surface,
                   vk::Extent2D windowExtent, Queue::FamilyIndices queues,
                   GLFWwindow *window, VkInstance rawInstance)
    : _device(device), _physical(physical), _surface(surface),
      _extent(windowExtent), _queues(queues), _window(window),
      _rawInstance(rawInstance),
      _swapchain(physical.get(), device.get(), surface.get(), windowExtent,
                 queues),
      _depth(physical.get(), device.get(), windowExtent),
      _renderPass(device.get(), _swapchain.imageFormat(), _depth.getFormat()),
      _cmdPool(device.get(), queues.graphics.value()) {

  std::vector<vk::DescriptorPoolSize> poolSizes = {
      {vk::DescriptorType::eSampler, 1000u},
      {vk::DescriptorType::eCombinedImageSampler, 1000u},
      {vk::DescriptorType::eSampledImage, 1000u},
      {vk::DescriptorType::eStorageImage, 1000u},
      {vk::DescriptorType::eUniformTexelBuffer, 1000u},
      {vk::DescriptorType::eStorageTexelBuffer, 1000u},
      {vk::DescriptorType::eUniformBuffer, 1000u},
      {vk::DescriptorType::eStorageBuffer, 1000u},
      {vk::DescriptorType::eUniformBufferDynamic, 1000u},
      {vk::DescriptorType::eStorageBufferDynamic, 1000u},
      {vk::DescriptorType::eInputAttachment, 1000u}};

  _imguiDescriptorPool = std::make_unique<vk::raii::DescriptorPool>(
      makeDescriptorPool(poolSizes, /*maxSets=*/1000u));

  uint32_t imgCount = static_cast<uint32_t>(_swapchain.images().size());

  imguiLayer_ = std::make_unique<ImGuiLayer>(
      _window, _rawInstance, static_cast<VkDevice>(*_device.get()),
      static_cast<VkPhysicalDevice>(*_physical.get()), _queues.graphics.value(),
      static_cast<VkQueue>(*_device.graphicsQueue()), *_imguiDescriptorPool,
      static_cast<VkRenderPass>(*_renderPass.get()), imgCount);

  //
  // — NEW: build everything else before first draw —
  //
  createCubeResources();

  buildFrameResources();

  // finally record your command‑buffers
  recordCommandBuffers();
}

void Renderer::recreateSwapchain() {
  cleanupSwapchain();
  createSwapchainResources();

  // ← same here
  buildFrameResources();

  recordCommandBuffers();
}

void Renderer::buildFrameResources() {
  createGraphicsPipeline();
  createFramebuffers();
  createCommandPoolAndBuffers();
  createSyncObjects();
}

void Renderer::createSwapchainResources() {

  VkSwapchainKHR oldSC = static_cast<VkSwapchainKHR>(*_swapchain.get());
  _swapchain = Swapchain(_physical.get(), _device.get(), _surface.get(),
                         _extent, _queues, oldSC);

  _depth = DepthBuffer(_physical.get(), _device.get(), _extent);

  _renderPass =
      RenderPass(_device.get(), _swapchain.imageFormat(), _depth.getFormat());

  uint32_t imgCount = static_cast<uint32_t>(_swapchain.images().size());

  imguiLayer_->recreate(imgCount,
                        static_cast<VkRenderPass>(*_renderPass.get()));
  createGraphicsPipeline();

  createFramebuffers();

  createCommandPoolAndBuffers();

  createSyncObjects();
}

void Renderer::cleanupSwapchain() {
  _device.get().waitIdle();

  _framebuffers.clear();
  _cmdBuffers.clear();

  _imageAvailable.clear();
  _renderFinished.clear();
  _inFlightFences.clear();
  _imagesInFlight.clear();

  _pipeline.reset();
  _pipelineLayout.reset();
}

void Renderer::onWindowResized(int newWidth, int newHeight) {
  if (newWidth == 0 || newHeight == 0)
    return;

  _extent = vk::Extent2D{uint32_t(newWidth), uint32_t(newHeight)};
  _framebufferResized = true;
}

void Renderer::drawFrame() {

  if (_framebufferResized) {
    recreateSwapchain();
    _framebufferResized = false;
  }

  VkFence fence = _inFlightFences[_currentFrame].get();
  vkWaitForFences(*_device.get(), 1, &fence, VK_TRUE, UINT64_MAX);
  _inFlightFences[_currentFrame].reset();

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(
      *_device.get(), *_swapchain.get(), UINT64_MAX,
      _imageAvailable[_currentFrame].get(), VK_NULL_HANDLE, &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapchain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("Failed to acquire swapchain image");
  }

  updateUniformBuffer(imageIndex);

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

  _currentFrame = (_currentFrame + 1) % _inFlightFences.size();
}

void Renderer::createCubeResources() {
  // — 1) cube vertex/index setup in CPU memory —
  _vertices = {
      {{-0.5f, -0.5f, -0.5f}, {1, 0, 0}}, {{0.5f, -0.5f, -0.5f}, {0, 1, 0}},
      {{0.5f, 0.5f, -0.5f}, {0, 0, 1}},   {{-0.5f, 0.5f, -0.5f}, {1, 1, 0}},
      {{-0.5f, -0.5f, 0.5f}, {1, 0, 1}},  {{0.5f, -0.5f, 0.5f}, {0, 1, 1}},
      {{0.5f, 0.5f, 0.5f}, {1, 1, 1}},    {{-0.5f, 0.5f, 0.5f}, {0, 0, 0}},
  };
  _indices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4, 0, 1, 5, 5, 4, 0,
              3, 2, 6, 6, 7, 3, 1, 2, 6, 6, 5, 1, 0, 3, 7, 7, 4, 0};

  // — 2) staging & copy into device‑local vertex/index buffers —
  vk::DeviceSize vbSize = sizeof(Vertex) * _vertices.size();
  vk::DeviceSize ibSize = sizeof(uint16_t) * _indices.size();
  Buffer stagingVB(_physical.get(), _device.get(), vbSize,
                   vk::BufferUsageFlagBits::eTransferSrc,
                   vk::MemoryPropertyFlagBits::eHostVisible |
                       vk::MemoryPropertyFlagBits::eHostCoherent);
  stagingVB.copyFrom(_vertices.data(), vbSize);
  Buffer stagingIB(_physical.get(), _device.get(), ibSize,
                   vk::BufferUsageFlagBits::eTransferSrc,
                   vk::MemoryPropertyFlagBits::eHostVisible |
                       vk::MemoryPropertyFlagBits::eHostCoherent);

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

  VkCommandBuffer c = beginSingleTimeCommands();
  VkBufferCopy r0{0, 0, vbSize};
  vkCmdCopyBuffer(c, *stagingVB.raw(), _vertexBuffer->get(), 1, &r0);
  VkBufferCopy r1{0, 0, ibSize};
  vkCmdCopyBuffer(c, *stagingIB.raw(), _indexBuffer->get(), 1, &r1);
  endSingleTimeCommands(c);

  // — 3) create UBO descriptor‑set‑layout (set 0) —
  vk::DescriptorSetLayoutBinding uboB{};
  uboB.binding = 0;
  uboB.descriptorType = vk::DescriptorType::eUniformBuffer;
  uboB.descriptorCount = 1;
  uboB.stageFlags = vk::ShaderStageFlagBits::eVertex;
  _uboSetLayout = std::make_unique<vk::raii::DescriptorSetLayout>(
      _device.get(),
      vk::DescriptorSetLayoutCreateInfo{}.setBindingCount(1).setPBindings(
          &uboB));

  // — 4) descriptor‑pool for UBO + SSBO (set 1) —
  uint32_t imgCount = uint32_t(_swapchain.images().size());
  vk::DescriptorPoolSize poolUBO{vk::DescriptorType::eUniformBuffer, imgCount};
  vk::DescriptorPoolSize poolSSBO{vk::DescriptorType::eStorageBuffer, 1};
  _descriptorPool = std::make_unique<vk::raii::DescriptorPool>(
      makeDescriptorPool({poolUBO, poolSSBO}, imgCount + 1));

  // allocate & write the UBO descriptor‑sets
  _uniformBuffers.resize(imgCount);
  for (size_t i = 0; i < imgCount; ++i) {
    _uniformBuffers[i] = std::make_unique<Buffer>(
        _physical.get(), _device.get(), sizeof(UniformBufferObject),
        vk::BufferUsageFlagBits::eUniformBuffer,
        vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent);
  }
  _descriptorSets.resize(imgCount);
  std::vector<VkDescriptorSetLayout> layouts(
      imgCount, VkDescriptorSetLayout(**_uboSetLayout));
  VkDescriptorSetAllocateInfo ainfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  ainfo.descriptorPool = VkDescriptorPool(**_descriptorPool);
  ainfo.descriptorSetCount = imgCount;
  ainfo.pSetLayouts = layouts.data();
  vkAllocateDescriptorSets(*_device.get(), &ainfo, _descriptorSets.data());
  for (uint32_t i = 0; i < imgCount; ++i) {
    VkDescriptorBufferInfo bi{_uniformBuffers[i]->get(), 0,
                              sizeof(UniformBufferObject)};
    VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    w.dstSet = _descriptorSets[i];
    w.dstBinding = 0;
    w.descriptorCount = 1;
    w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w.pBufferInfo = &bi;
    vkUpdateDescriptorSets(*_device.get(), 1, &w, 0, nullptr);
  }

  // — 5) now that pool, layouts & buffers exist, build the terrain SSBO —
  const int width = 16, depth = 16, cubeSize = 1, brickDim = 4;
  _terrain = TerrainGenerator::createFlatTerrain(
      width, depth, cubeSize, brickDim, _device.get(), _physical.get(),
      VkDescriptorPool(**_descriptorPool),
      VkDescriptorSetLayout(**_uboSetLayout), *_vertexBuffer, *_indexBuffer);

  // grab the voxel‐set‐layout from what the terrain gave us
  _voxelSetLayout = std::move(_terrain.voxelResources.layout);
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
    std::array<vk::ImageView, 2> atts = {*_colorImageViews[i].get(),
                                         *_depth.getView()};
    _framebuffers.emplace_back(
        _device.get(), _renderPass.get(), _extent,
        std::vector<vk::ImageView>{atts.begin(), atts.end()});
  }
}

void Renderer::updateUniformBuffer(uint32_t currentImage) {
  static auto startTime = std::chrono::high_resolution_clock::now();
  auto now = std::chrono::high_resolution_clock::now();
  float time = std::chrono::duration<float, std::chrono::seconds::period>(
                   now - startTime)
                   .count();

  UniformBufferObject ubo{};

  ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f),
                          glm::vec3(1, 1, 0));

  ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f),
                         glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.proj = glm::perspective(
      glm::radians(45.0f), _extent.width / float(_extent.height), 0.1f, 10.0f);
  ubo.proj[1][1] *= -1;

  // copy into the mapped buffer
  void *data = _uniformBuffers[currentImage]->map();
  std::memcpy(data, &ubo, sizeof(ubo));
  _uniformBuffers[currentImage]->unmap();
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

  // set the two descriptor‑set layouts: 0=UBO, 1=voxel SSBO
  cfg.setLayouts = {static_cast<vk::DescriptorSetLayout>(**_uboSetLayout),
                    static_cast<vk::DescriptorSetLayout>(**_voxelSetLayout)};

  //
  // — vertex‑input binding descriptions —
  //
  // binding 0 already populated by defaultPipelineConfig:
  //   cfg.bindingDescriptions[0] = Vertex::getBindingDescription();
  // now set up binding 1 for per‑instance mat4:
  vk::VertexInputBindingDescription instBind{};
  instBind.binding = 1;
  instBind.stride = sizeof(glm::mat4);
  instBind.inputRate = vk::VertexInputRate::eInstance;
  cfg.bindingDescriptions[1] = instBind;

  // now update the vertexInput create‑info to use both bindings:
  cfg.vertexInput.setVertexBindingDescriptionCount(2)
      .setPVertexBindingDescriptions(cfg.bindingDescriptions.data());

  //
  // — vertex‑input attribute descriptions —
  //
  // original per‑vertex attributes in cfg.attributeDescriptions
  auto &origAttrs = cfg.attributeDescriptions;

  // per‑instance mat4 → 4 × vec4 at locations 2,3,4,5
  std::array<vk::VertexInputAttributeDescription, 4> instAttrs = {
      {{/*location*/ 2, /*binding*/ 1, vk::Format::eR32G32B32A32Sfloat,
        /*offset*/ 0},
       {3, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4)},
       {4, 1, vk::Format::eR32G32B32A32Sfloat, 2 * sizeof(glm::vec4)},
       {5, 1, vk::Format::eR32G32B32A32Sfloat, 3 * sizeof(glm::vec4)}}};

  // merge them
  std::vector<vk::VertexInputAttributeDescription> allAttrs;
  allAttrs.reserve(origAttrs.size() + instAttrs.size());
  allAttrs.insert(allAttrs.end(), origAttrs.begin(), origAttrs.end());
  allAttrs.insert(allAttrs.end(), instAttrs.begin(), instAttrs.end());

  // patch the create‑info
  cfg.vertexInput.setVertexAttributeDescriptionCount((uint32_t)allAttrs.size())
      .setPVertexAttributeDescriptions(allAttrs.data());

  // — shaders —
  ShaderModule vert{_device.get(), "shaders/triangle.vert.spv"};
  ShaderModule frag{_device.get(), "shaders/triangle.frag.spv"};
  std::array<vk::PipelineShaderStageCreateInfo, 2> stages = {
      vert.stageInfo(vk::ShaderStageFlagBits::eVertex),
      frag.stageInfo(vk::ShaderStageFlagBits::eFragment)};

  // — layout & pipeline creation —
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

    // 1) begin command‐buffer
    vk::CommandBufferBeginInfo beginInfo{};
    cmd.begin(beginInfo);

    // 2) begin render‐pass
    vk::RenderPassBeginInfo rpInfo{};
    rpInfo.setRenderPass(*_renderPass.get())
        .setFramebuffer(*_framebuffers[i].get())
        .setRenderArea(vk::Rect2D{{0, 0}, _extent});
    std::array<vk::ClearValue, 2> clears{};
    clears[0].color = vk::ClearColorValue(std::array<float, 4>{0, 0, 0, 1});
    clears[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
    rpInfo.setClearValueCount((uint32_t)clears.size())
        .setPClearValues(clears.data());
    cmd.beginRenderPass(rpInfo, vk::SubpassContents::eInline);

    // 3) bind pipeline
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *_pipeline->get());

    // 4) bind vertex + instance buffers
    vk::Buffer vbs[] = {_vertexBuffer->get(), _terrain.instanceBuffer->get()};
    vk::DeviceSize offs[] = {0, 0};
    cmd.bindVertexBuffers(0, 2, vbs, offs);

    // 5) bind index buffer
    cmd.bindIndexBuffer(_indexBuffer->get(), 0, vk::IndexType::eUint16);

    // 6) bind descriptor sets (UBO = set 0, SSBO = set 1)
    std::array<VkDescriptorSet, 2> sets = {
        _descriptorSets[i], _terrain.voxelResources.descriptorSet};
    vkCmdBindDescriptorSets(
        static_cast<VkCommandBuffer>(cmd), VK_PIPELINE_BIND_POINT_GRAPHICS,
        static_cast<VkPipelineLayout>(*_pipelineLayout->get()), 0,
        (uint32_t)sets.size(), sets.data(), 0, nullptr);

    // 7) draw instanced
    cmd.drawIndexed((uint32_t)_indices.size(), _terrain.instanceCount, 0, 0, 0);

    // 8) render ImGui on top
    imguiLayer_->render(cmd);

    // 9) end render‐pass + finish
    cmd.endRenderPass();
    cmd.end();
  }
}
void Renderer::createCommandPoolAndBuffers() {
  _cmdBuffers.clear();
  _cmdBuffers.reserve(_framebuffers.size());
  for (size_t i = 0; i < _framebuffers.size(); ++i) {
    _cmdBuffers.emplace_back(_device.get(), _cmdPool.get());
  }
}

VkCommandBuffer Renderer::beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = *_cmdPool.get();
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer cmd;
  vkAllocateCommandBuffers(*_device.get(), &allocInfo, &cmd);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd, &beginInfo);

  return cmd;
}

void Renderer::endSingleTimeCommands(VkCommandBuffer cmd) {
  vkEndCommandBuffer(cmd);

  VkSubmitInfo submit{};
  submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &cmd;

  vkQueueSubmit(*_device.graphicsQueue(), 1, &submit, VK_NULL_HANDLE);
  vkQueueWaitIdle(*_device.graphicsQueue());

  vkFreeCommandBuffers(*_device.get(), *_cmdPool.get(), 1, &cmd);
}

vk::raii::DescriptorPool Renderer::makeDescriptorPool(
    const std::vector<vk::DescriptorPoolSize> &poolSizes, uint32_t maxSets) {
  vk::DescriptorPoolCreateInfo info;
  info.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
      .setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
      .setPPoolSizes(poolSizes.data())
      .setMaxSets(maxSets);

  return vk::raii::DescriptorPool{_device.get(), info};
}

} // namespace engine
