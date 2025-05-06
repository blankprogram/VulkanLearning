
#include "engine/app/Renderer.hpp"
#include "engine/configs/PipelineConfig.hpp"
#include "engine/pipeline/ShaderModule.hpp"
#include "engine/swapchain/ImageView.hpp" // <–– add this

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
  // 1) wait for *this frame’s* fence and then reset it
  VkFence currentFence = _inFlightFences[_currentFrame].get();
  vkWaitForFences(*_device.get(), 1, &currentFence, VK_TRUE, UINT64_MAX);
  _inFlightFences[_currentFrame].reset();

  // 2) acquire next image
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

  // === NEW BLOCK #1 ===
  // if that image is already in flight, wait on its associated fence
  if (_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
    vkWaitForFences(*_device.get(), 1, &_imagesInFlight[imageIndex], VK_TRUE,
                    UINT64_MAX);
  }
  // now mark this image as being “in flight” on our current fence
  _imagesInFlight[imageIndex] = currentFence;
  // ===================

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
  VkCommandBuffer rawCmdBuf = _cmdBuffers[imageIndex].get();
  submitInfo.pCommandBuffers = &rawCmdBuf;
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

  // 5) advance to next “frame index”
  _currentFrame = (_currentFrame + 1) % _inFlightFences.size();
}

void Renderer::createFramebuffers() {
  // 1) build + keep alive our color‐attachment views
  _colorImageViews.clear();
  auto const &images = _swapchain.images();
  _colorImageViews.reserve(images.size());
  for (auto img : images) {
    _colorImageViews.emplace_back(_device.get(), img, _swapchain.imageFormat(),
                                  vk::ImageAspectFlagBits::eColor, 1, 1);
  }

  // 2) now assemble framebuffers using those live wrappers + depth view
  _framebuffers.clear();
  _framebuffers.reserve(images.size());
  for (size_t i = 0; i < images.size(); ++i) {
    std::array<vk::ImageView, 2> attachments = {
        *(_colorImageViews[i].get()), // raw VkImageView handle
        *(_depth.getView())           // raw VkImageView handle
    };

    _framebuffers.emplace_back(
        _device.get(), _renderPass.get(), _extent,
        std::vector<vk::ImageView>{attachments.begin(), attachments.end()});
  }
}

void Renderer::createSyncObjects() {
  size_t n = _framebuffers.size();
  _imageAvailable.clear();
  _renderFinished.clear();
  _inFlightFences.clear();
  _imageAvailable.reserve(n);
  _renderFinished.reserve(n);
  _inFlightFences.reserve(n);

  for (size_t i = 0; i < n; ++i) {
    _imageAvailable.emplace_back(_device.get());
    _renderFinished.emplace_back(_device.get());
    _inFlightFences.emplace_back(_device.get(), true);
    _imagesInFlight.assign(n, VK_NULL_HANDLE);
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

  _pipeline = std::make_unique<GraphicsPipeline>(
      _device.get(),
      PipelineLayout{_device.get(), cfg.setLayouts, cfg.pushConstants},
      _renderPass.get(), cfg.vertexInput, cfg.inputAssembly, cfg.rasterizer,
      cfg.multisampling, cfg.depthStencil, cfg.colorBlend,
      std::vector<vk::PipelineShaderStageCreateInfo>{stages.begin(),
                                                     stages.end()},
      std::vector<vk::PipelineViewportStateCreateInfo>{cfg.viewportState},
      cfg.dynamicState,
      GraphicsPipeline::Config{cfg.viewportExtent, cfg.msaaSamples});
}

void Renderer::recordCommandBuffers() {
  for (size_t i = 0; i < _cmdBuffers.size(); ++i) {
    auto cmd = _cmdBuffers[i].get();
    vk::CommandBufferBeginInfo bi{};
    cmd.begin(bi);

    // two clear‐values: 0=color, 1=depth
    std::array<vk::ClearValue, 2> clearValues = {
        vk::ClearColorValue{std::array<float, 4>{0.1f, 0.2f, 0.3f, 1.0f}},
        vk::ClearDepthStencilValue{1.0f, 0}};

    vk::RenderPassBeginInfo rpbi{};
    rpbi.setRenderPass(*_renderPass.get())
        .setFramebuffer(*_framebuffers[i].get())
        .setRenderArea({{0, 0}, _extent})
        .setClearValueCount(static_cast<uint32_t>(clearValues.size()))
        .setPClearValues(clearValues.data());

    cmd.beginRenderPass(rpbi, vk::SubpassContents::eInline);
    // (no draws yet)
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

} // namespace engine
