
#include "engine/platform/RendererContext.hpp"
#include <stdexcept>

RendererContext::RendererContext(GLFWwindow *window) { init(window); }

RendererContext::~RendererContext() { cleanup(); }

void RendererContext::init(GLFWwindow *window) {
  device_ = std::make_unique<VulkanDevice>(window);
  swapchain_ = std::make_unique<Swapchain>(device_.get(), device_->surface);
  frameSync_.init(device_->device, MAX_FRAMES_IN_FLIGHT);
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = device_->commandPool; // <- Add accessor if needed
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

  if (vkAllocateCommandBuffers(device_->device, &allocInfo, commandBuffers_) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate command buffers!");
  }
}

void RendererContext::beginFrame() {
  VkFence inFlightFence = frameSync_.getInFlightFence(currentFrame_);
  vkWaitForFences(device_->device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
  vkResetFences(device_->device, 1, &inFlightFence);

  VkResult result = vkAcquireNextImageKHR(
      device_->device, swapchain_->getSwapchain(), UINT64_MAX,
      frameSync_.getImageAvailable(currentFrame_), VK_NULL_HANDLE,
      &currentImageIndex_);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapchain();
    return;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  // Command buffer recording should happen here
}

void RendererContext::endFrame() {
  // Replace with actual command buffer from your command pool system
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE; // <- Replace with actual one

  VkSemaphore waitSemaphores[] = {frameSync_.getImageAvailable(currentFrame_)};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore signalSemaphores[] = {
      frameSync_.getRenderFinished(currentFrame_)};

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer; // Provide valid command buffer
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  VkFence inFlightFence = frameSync_.getInFlightFence(currentFrame_);

  if (vkQueueSubmit(device_->graphicsQueue, 1, &submitInfo, inFlightFence) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapchains[] = {swapchain_->getSwapchain()};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapchains;
  presentInfo.pImageIndices = &currentImageIndex_;
  presentInfo.pResults = nullptr;

  VkResult result = vkQueuePresentKHR(device_->graphicsQueue, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    recreateSwapchain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image!");
  }

  currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RendererContext::recreateSwapchain() {
  vkDeviceWaitIdle(device_->device);
  swapchain_->cleanup();
  swapchain_->recreate();
}

void RendererContext::cleanup() {
  frameSync_.cleanup(device_->device);
  swapchain_->cleanup();
  swapchain_.reset();
  device_.reset();
}
