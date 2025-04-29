
// src/platform/RendererContext.cpp
#include "engine/platform/RendererContext.hpp"
#include "engine/platform/DescriptorManager.hpp"
#include "engine/utils/VulkanHelpers.hpp"
#include <array>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

// Make sure these match your window size constants, or query at runtime:
static constexpr uint32_t WIDTH = 1280;
static constexpr uint32_t HEIGHT = 720;

RendererContext::RendererContext(GLFWwindow *window)
    : cam_(glm::radians(45.0f), float(WIDTH) / float(HEIGHT), 0.1f, 100.0f) {
  init(window);
  createUniforms();
}

RendererContext::~RendererContext() { cleanup(); }

void RendererContext::init(GLFWwindow *window) {
  // 1) device, swapchain, sync objects
  device_ = std::make_unique<VulkanDevice>(window);
  swapchain_ =
      std::make_unique<Swapchain>(device_.get(), device_->surface, window);
  frameSync_.init(device_->device, MAX_FRAMES_IN_FLIGHT);

  extent_ = swapchain_->getExtent();
  createRenderPass();
  createFramebuffers();

  // we'll call createUniforms() before pipeline so the descriptor layout is
  // ready
  createUniforms();

  pipeline_.init(device_->device, renderPass_, descriptorMgr_.getLayout(),
                 std::string(SPIRV_OUT) + "/vert.spv",
                 std::string(SPIRV_OUT) + "/frag.spv");

  // 2) allocate command buffers
  VkCommandBufferAllocateInfo allocInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandPool = device_->commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
  if (vkAllocateCommandBuffers(device_->device, &allocInfo, commandBuffers_) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate command buffers!");
  }
}

void RendererContext::createUniforms() {
  VkDevice dev = device_->device;
  VkPhysicalDevice phys = device_->physicalDevice;

  // allocate one big HOST_VISIBLE UBO for all frames
  VkDeviceSize totalSize = sizeof(glm::mat4) * MAX_FRAMES_IN_FLIGHT;
  engine::utils::CreateHostVisibleBuffer(dev, phys, totalSize,
                                         VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         uniformBuffer_, uniformMemory_);

  // descriptor: only one set, dynamic
  descriptorMgr_.init(dev, 1);

  VkDescriptorBufferInfo bi{};
  bi.buffer = uniformBuffer_;
  bi.offset = 0;
  bi.range = sizeof(glm::mat4);

  VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  w.dstSet = descriptorMgr_.getDescriptorSets()[0];
  w.dstBinding = 0;
  w.dstArrayElement = 0;
  w.descriptorCount = 1;
  w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  w.pBufferInfo = &bi;
  vkUpdateDescriptorSets(dev, 1, &w, 0, nullptr);
}

void RendererContext::beginFrame() {
  // wait / acquire
  VkFence fence = frameSync_.getInFlightFence(currentFrame_);
  vkWaitForFences(device_->device, 1, &fence, VK_TRUE, UINT64_MAX);
  vkResetFences(device_->device, 1, &fence);

  VkResult res = vkAcquireNextImageKHR(
      device_->device, swapchain_->getSwapchain(), UINT64_MAX,
      frameSync_.getImageAvailable(currentFrame_), VK_NULL_HANDLE,
      &currentImageIndex_);

  if (res == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapchain();
    return;
  } else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("Failed to acquire swapchain image!");
  }

  // update UBO for this frame
  glm::mat4 viewProj = cam_.viewProjection();
  void *ptr = nullptr;
  VkDeviceSize offset = sizeof(glm::mat4) * currentFrame_;
  vkMapMemory(device_->device, uniformMemory_, offset, sizeof(viewProj), 0,
              &ptr);
  std::memcpy(ptr, glm::value_ptr(viewProj), sizeof(viewProj));
  vkUnmapMemory(device_->device, uniformMemory_);

  // record commands
  currentCommandBuffer_ = commandBuffers_[currentFrame_];
  VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(currentCommandBuffer_, &bi);

  VkClearValue clear{.color = {{0.1f, 0.1f, 0.1f, 1.0f}}};
  VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  rpbi.renderPass = renderPass_;
  rpbi.framebuffer = framebuffers_[currentImageIndex_];
  rpbi.renderArea = {{0, 0}, extent_};
  rpbi.clearValueCount = 1;
  rpbi.pClearValues = &clear;
  vkCmdBeginRenderPass(currentCommandBuffer_, &rpbi,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(currentCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_.pipeline);

  // bind the single dynamic UBO with frame-specific offset
  uint32_t dynOffset = uint32_t(offset);
  vkCmdBindDescriptorSets(currentCommandBuffer_,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.layout,
                          0, // firstSet
                          1, // descriptorCount
                          &descriptorMgr_.getDescriptorSets()[0],
                          1, // dynamicOffsetCount
                          &dynOffset);

  VkViewport vp{0.f, 0.f, float(extent_.width), float(extent_.height),
                0.f, 1.f};
  vkCmdSetViewport(currentCommandBuffer_, 0, 1, &vp);
  VkRect2D sc{{0, 0}, extent_};
  vkCmdSetScissor(currentCommandBuffer_, 0, 1, &sc);
}

void RendererContext::endFrame() {
  vkCmdEndRenderPass(currentCommandBuffer_);
  vkEndCommandBuffer(currentCommandBuffer_);

  VkSemaphore waitSems[] = {frameSync_.getImageAvailable(currentFrame_)};
  VkPipelineStageFlags ps[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore sigSems[] = {frameSync_.getRenderFinished(currentFrame_)};

  VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  si.waitSemaphoreCount = 1;
  si.pWaitSemaphores = waitSems;
  si.pWaitDstStageMask = ps;
  si.commandBufferCount = 1;
  si.pCommandBuffers = &currentCommandBuffer_;
  si.signalSemaphoreCount = 1;
  si.pSignalSemaphores = sigSems;

  vkQueueSubmit(device_->graphicsQueue, 1, &si,
                frameSync_.getInFlightFence(currentFrame_));

  VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  pi.waitSemaphoreCount = 1;
  pi.pWaitSemaphores = sigSems;
  VkSwapchainKHR scs[] = {swapchain_->getSwapchain()};
  pi.swapchainCount = 1;
  pi.pSwapchains = scs;
  pi.pImageIndices = &currentImageIndex_;

  VkResult pres = vkQueuePresentKHR(device_->graphicsQueue, &pi);
  if (pres == VK_ERROR_OUT_OF_DATE_KHR || pres == VK_SUBOPTIMAL_KHR) {
    recreateSwapchain();
  } else if (pres != VK_SUCCESS) {
    throw std::runtime_error("Failed to present swapchain image!");
  }

  currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RendererContext::createFramebuffers() {
  for (auto fb : framebuffers_)
    vkDestroyFramebuffer(device_->device, fb, nullptr);
  framebuffers_.clear();

  auto &views = swapchain_->getImageViews();
  framebuffers_.resize(views.size());
  for (size_t i = 0; i < views.size(); ++i) {
    VkImageView attachments[] = {views[i]};
    VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fci.renderPass = renderPass_;
    fci.attachmentCount = 1;
    fci.pAttachments = attachments;
    fci.width = extent_.width;
    fci.height = extent_.height;
    fci.layers = 1;
    if (vkCreateFramebuffer(device_->device, &fci, nullptr,
                            &framebuffers_[i]) != VK_SUCCESS)
      throw std::runtime_error("Failed to create framebuffer");
  }
}

void RendererContext::recreateSwapchain() {
  vkDeviceWaitIdle(device_->device);
  swapchain_->cleanup();
  for (auto fb : framebuffers_)
    vkDestroyFramebuffer(device_->device, fb, nullptr);
  vkDestroyRenderPass(device_->device, renderPass_, nullptr);
  pipeline_.cleanup(device_->device);

  swapchain_->recreate();
  extent_ = swapchain_->getExtent();
  createRenderPass();
  createFramebuffers();
  pipeline_.init(device_->device, renderPass_, descriptorMgr_.getLayout(),
                 std::string(SPIRV_OUT) + "/vert.spv",
                 std::string(SPIRV_OUT) + "/frag.spv");
}

void RendererContext::cleanup() {
  vkDestroyBuffer(device_->device, uniformBuffer_, nullptr);
  vkFreeMemory(device_->device, uniformMemory_, nullptr);

  descriptorMgr_.cleanup(device_->device);
  frameSync_.cleanup(device_->device);

  for (auto fb : framebuffers_)
    vkDestroyFramebuffer(device_->device, fb, nullptr);
  vkDestroyRenderPass(device_->device, renderPass_, nullptr);
  swapchain_.reset();
  device_.reset();
}

void RendererContext::createRenderPass() {
  auto dev = device_->device;
  VkAttachmentDescription colorAtt{};
  colorAtt.format = swapchain_->getImageFormat();
  colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;

  VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  rpci.attachmentCount = 1;
  rpci.pAttachments = &colorAtt;
  rpci.subpassCount = 1;
  rpci.pSubpasses = &subpass;

  if (vkCreateRenderPass(dev, &rpci, nullptr, &renderPass_) != VK_SUCCESS)
    throw std::runtime_error("Failed to create render pass");
}
