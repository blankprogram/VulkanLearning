
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
  allocator_ = device_->getAllocator();

  frameSync_.init(device_->getDevice(), MAX_FRAMES_IN_FLIGHT);
  commandManager_.init(device_->getDevice(),
                       device_->getGraphicsQueueFamilyIndex(),
                       MAX_FRAMES_IN_FLIGHT);
  renderResources_.init(device_.get(), swapchain_.get());
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

  VkImageLayout layout = renderGraph_.getFinalLayout(); // from previous frame
  if (layout == VK_IMAGE_LAYOUT_UNDEFINED)
    layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  renderGraph_.beginFrame(
      renderResources_, commandManager_, currentFrame_, viewProj,
      renderResources_.getSwapchain()->getImages()[currentImageIndex_], layout);
}

void RendererContext::endFrame() {
  renderGraph_.endFrame();

  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  VkSemaphore waitSems[] = {frameSync_.getImageAvailable(currentFrame_)};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore signalSems[] = {frameSync_.getRenderFinished(currentFrame_)};

  submit.waitSemaphoreCount = 1;
  submit.pWaitSemaphores = waitSems;
  submit.pWaitDstStageMask = waitStages;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &renderGraph_.getCurrentCommandBuffer();
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
  cam_.setAspect(float(swapchain_->getExtent().width) /
                 float(swapchain_->getExtent().height));
  renderResources_.recreate();
}

void RendererContext::cleanup() {
  vkDeviceWaitIdle(device_->getDevice());
  renderResources_.cleanup();
  commandManager_.cleanup(device_->getDevice());
  frameSync_.cleanup(device_->getDevice());
  swapchain_->cleanup();
  vmaDestroyAllocator(allocator_);
  device_.reset();
}
