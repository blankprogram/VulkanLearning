
#include "engine/platform/RendererContext.hpp"
#include "engine/utils/VulkanHelpers.hpp"
#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#define VMA_IMPLEMENTATION
#include "externals/vk_mem_alloc.h"
RendererContext::RendererContext(GLFWwindow *window)
    : cam_(glm::radians(45.0f), 1280.f / 720.f, 0.1f, 100.0f) {
  init(window);
}

RendererContext::~RendererContext() { cleanup(); }

void RendererContext::init(GLFWwindow *window) {
  device_ = std::make_unique<VulkanDevice>(window);
  swapchain_ =
      std::make_unique<Swapchain>(device_.get(), device_->getSurface(), window);
  frameSync_.init(device_->getDevice(), MAX_FRAMES_IN_FLIGHT);

  allocator_ = device_->getAllocator();

  // Setup rendering resources
  VkExtent2D extent = swapchain_->getExtent();
  VkFormat colorFormat = swapchain_->getImageFormat();

  depthResources_.init(device_->getDevice(), allocator_,
                       device_->getPhysicalDevice(), extent,
                       VK_FORMAT_D32_SFLOAT);
  renderPassMgr_.init(device_->getDevice(), colorFormat,
                      depthResources_.format());

  uniformMgr_.init(device_->getDevice(), allocator_, MAX_FRAMES_IN_FLIGHT);
  pipeline_.init(device_->getDevice(), renderPassMgr_.get(),
                 uniformMgr_.layout(), std::string(SPIRV_OUT) + "/vert.spv",
                 std::string(SPIRV_OUT) + "/frag.spv");

  framebufferMgr_.init(device_->getDevice(), renderPassMgr_.get(), extent,
                       swapchain_->getImageViews(), depthResources_.view());

  // Allocate command buffers
  VkCommandBufferAllocateInfo allocInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandPool = device_->getCommandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
  vkAllocateCommandBuffers(device_->getDevice(), &allocInfo, commandBuffers_);
}

void RendererContext::beginFrame() {
  VkDevice dev = device_->getDevice();
  VkFence fence = frameSync_.getInFlightFence(currentFrame_);
  vkWaitForFences(dev, 1, &fence, VK_TRUE, UINT64_MAX);
  vkResetFences(dev, 1, &fence);

  VkResult result =
      vkAcquireNextImageKHR(dev, swapchain_->getSwapchain(), UINT64_MAX,
                            frameSync_.getImageAvailable(currentFrame_),
                            VK_NULL_HANDLE, &currentImageIndex_);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapchain();
    return;
  }

  glm::mat4 viewProj = cam_.viewProjection();
  uniformMgr_.update(allocator_, currentFrame_, viewProj);

  currentCommandBuffer_ = commandBuffers_[currentFrame_];

  VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(currentCommandBuffer_, &begin);

  VkClearValue clears[2] = {};
  clears[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
  clears[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo rpBegin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  rpBegin.renderPass = renderPassMgr_.get();
  rpBegin.framebuffer = framebufferMgr_.get()[currentImageIndex_];
  rpBegin.renderArea = {{0, 0}, swapchain_->getExtent()};
  rpBegin.clearValueCount = 2;
  rpBegin.pClearValues = clears;

  vkCmdBeginRenderPass(currentCommandBuffer_, &rpBegin,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(currentCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_.pipeline);
  uint32_t dynOffset = static_cast<uint32_t>(sizeof(glm::mat4) * currentFrame_);
  vkCmdBindDescriptorSets(currentCommandBuffer_,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.layout, 0,
                          1, &uniformMgr_.sets()[0], 1, &dynOffset);

  VkViewport vp{0.f,
                0.f,
                float(swapchain_->getExtent().width),
                float(swapchain_->getExtent().height),
                0.f,
                1.f};
  VkRect2D scissor{{0, 0}, swapchain_->getExtent()};
  vkCmdSetViewport(currentCommandBuffer_, 0, 1, &vp);
  vkCmdSetScissor(currentCommandBuffer_, 0, 1, &scissor);
}

void RendererContext::endFrame() {
  vkCmdEndRenderPass(currentCommandBuffer_);
  vkEndCommandBuffer(currentCommandBuffer_);

  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  VkSemaphore waitSems[] = {frameSync_.getImageAvailable(currentFrame_)};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore signalSems[] = {frameSync_.getRenderFinished(currentFrame_)};

  submit.waitSemaphoreCount = 1;
  submit.pWaitSemaphores = waitSems;
  submit.pWaitDstStageMask = waitStages;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &currentCommandBuffer_;
  submit.signalSemaphoreCount = 1;
  submit.pSignalSemaphores = signalSems;

  vkQueueSubmit(device_->getGraphicsQueue(), 1, &submit,
                frameSync_.getInFlightFence(currentFrame_));

  VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  present.waitSemaphoreCount = 1;
  present.pWaitSemaphores = signalSems;
  present.swapchainCount = 1;
  VkSwapchainKHR sc = swapchain_->getSwapchain();
  present.pSwapchains = &sc;
  present.pImageIndices = &currentImageIndex_;

  VkResult result = vkQueuePresentKHR(device_->getGraphicsQueue(), &present);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    recreateSwapchain();
  }

  currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RendererContext::recreateSwapchain() {
  vkDeviceWaitIdle(device_->getDevice());
  swapchain_->recreate();
  VkExtent2D extent = swapchain_->getExtent();
  cam_.setAspect(float(extent.width) / float(extent.height));

  depthResources_.cleanup(device_->getDevice(), allocator_);
  depthResources_.init(device_->getDevice(), allocator_,
                       device_->getPhysicalDevice(), extent,
                       VK_FORMAT_D32_SFLOAT);

  renderPassMgr_.cleanup(device_->getDevice());
  renderPassMgr_.init(device_->getDevice(), swapchain_->getImageFormat(),
                      depthResources_.format());

  pipeline_.cleanup(device_->getDevice());
  pipeline_.init(device_->getDevice(), renderPassMgr_.get(),
                 uniformMgr_.layout(), std::string(SPIRV_OUT) + "/vert.spv",
                 std::string(SPIRV_OUT) + "/frag.spv");

  framebufferMgr_.cleanup(device_->getDevice());
  framebufferMgr_.init(device_->getDevice(), renderPassMgr_.get(), extent,
                       swapchain_->getImageViews(), depthResources_.view());
}

void RendererContext::cleanup() {
  framebufferMgr_.cleanup(device_->getDevice());
  renderPassMgr_.cleanup(device_->getDevice());
  depthResources_.cleanup(device_->getDevice(), allocator_);
  uniformMgr_.cleanup(device_->getDevice(), allocator_);
  pipeline_.cleanup(device_->getDevice());
  frameSync_.cleanup(device_->getDevice());
  swapchain_->cleanup();
  vmaDestroyAllocator(allocator_);
  device_.reset();
}
