
// src/platform/RendererContext.cpp
#include "engine/platform/RendererContext.hpp"
#include "engine/platform/DescriptorManager.hpp"
#include "engine/utils/VulkanHelpers.hpp"
#include <array>
#include <cstring>
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

  extent_ = swapchain_->getExtent(); // store for viewport/scissor
  createRenderPass();
  createFramebuffers();

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

  // (Later: create render pass & pipeline here)
}

void RendererContext::createUniforms() {
  VkDevice device = device_->device;
  uint32_t n = MAX_FRAMES_IN_FLIGHT;

  // 1) create one UBO per frame
  uniformBuffers_.resize(n);
  uniformMemories_.resize(n);
  for (uint32_t i = 0; i < n; ++i) {
    // allocate a host‐visible buffer of sizeof(mat4)
    engine::utils::CreateBuffer(device, device_->physicalDevice,
                                device_->commandPool, device_->graphicsQueue,
                                nullptr, // no initial data
                                sizeof(glm::mat4),
                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                uniformBuffers_[i], uniformMemories_[i]);
  }

  // 2) descriptor layout / pool / sets
  descriptorMgr_.init(device, n);

  // 3) write each descriptor to point at its UBO
  for (uint32_t i = 0; i < n; ++i) {
    VkDescriptorBufferInfo bi{};
    bi.buffer = uniformBuffers_[i];
    bi.offset = 0;
    bi.range = sizeof(glm::mat4);

    VkWriteDescriptorSet w{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    w.dstSet = descriptorMgr_.getDescriptorSets()[i];
    w.dstBinding = 0;
    w.dstArrayElement = 0;
    w.descriptorCount = 1;
    w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    w.pBufferInfo = &bi;

    vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);
  }
}

void RendererContext::beginFrame() {
  // wait on fence for this frame
  VkFence fence = frameSync_.getInFlightFence(currentFrame_);
  vkWaitForFences(device_->device, 1, &fence, VK_TRUE, UINT64_MAX);
  vkResetFences(device_->device, 1, &fence);

  // acquire next image
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
  vkMapMemory(device_->device, uniformMemories_[currentFrame_], 0,
              sizeof(viewProj), 0, &ptr);
  std::memcpy(ptr, &viewProj, sizeof(viewProj));
  vkUnmapMemory(device_->device, uniformMemories_[currentFrame_]);

  // start the render‐pass
  VkClearValue clear{.color = {{0.1f, 0.1f, 0.1f, 1.0f}}};
  VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  rpbi.renderPass = renderPass_;
  rpbi.framebuffer = framebuffers_[currentImageIndex_];
  rpbi.renderArea = {{0, 0}, extent_};
  rpbi.clearValueCount = 1;
  rpbi.pClearValues = &clear;
  vkCmdBeginRenderPass(currentCommandBuffer_, &rpbi,
                       VK_SUBPASS_CONTENTS_INLINE);

  // bind your pipeline + descriptor set
  vkCmdBindPipeline(currentCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_.pipeline);
  vkCmdBindDescriptorSets(
      currentCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.layout,
      0, 1, &descriptorMgr_.getDescriptorSets()[currentImageIndex_], 0,
      nullptr);

  // dynamic viewport + scissor
  VkViewport vp{0, 0, float(extent_.width), float(extent_.height), 0.0f, 1.0f};
  vkCmdSetViewport(currentCommandBuffer_, 0, 1, &vp);
  VkRect2D sc{{0, 0}, extent_};
  vkCmdSetScissor(currentCommandBuffer_, 0, 1, &sc);

  // now your ChunkRenderSystem::drawAll() will emit vkCmdDrawIndexed()...
  // begin recording
  currentCommandBuffer_ = commandBuffers_[currentFrame_];
  CommandBufferRecorder recorder(currentCommandBuffer_);
}

void RendererContext::endFrame() {
  VkCommandBuffer cmd = currentCommandBuffer_;

  // submit
  VkSemaphore waitSems[] = {frameSync_.getImageAvailable(currentFrame_)};
  VkPipelineStageFlags ps[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore sigSems[] = {frameSync_.getRenderFinished(currentFrame_)};
  VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  si.waitSemaphoreCount = 1;
  si.pWaitSemaphores = waitSems;
  si.pWaitDstStageMask = ps;
  si.commandBufferCount = 1;
  si.pCommandBuffers = &cmd;
  si.signalSemaphoreCount = 1;
  si.pSignalSemaphores = sigSems;

  vkQueueSubmit(device_->graphicsQueue, 1, &si,
                frameSync_.getInFlightFence(currentFrame_));

  // present
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

  // advance frame index
  currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RendererContext::createFramebuffers() {
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
                            &framebuffers_[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create framebuffer");
    }
  }
}

void RendererContext::recreateSwapchain() {
  vkDeviceWaitIdle(device_->device);
  swapchain_->cleanup();

  // destroy old
  for (auto fb : framebuffers_)
    vkDestroyFramebuffer(device_->device, fb, nullptr);
  vkDestroyRenderPass(device_->device, renderPass_, nullptr);
  pipeline_.cleanup(device_->device);

  // now recreate:
  swapchain_->cleanup();
  swapchain_->recreate();
  extent_ = swapchain_->getExtent();
  createRenderPass();
  createFramebuffers();
  std::string shaderDir = "/assets/shaders"; // or wherever CMake writes them
  pipeline_.init(device_->device, renderPass_, descriptorMgr_.getLayout(),
                 shaderDir + "/vert.spv", shaderDir + "/frag.spv");
  swapchain_->recreate();
}

void RendererContext::cleanup() {
  // destroy all UBO buffers
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroyBuffer(device_->device, uniformBuffers_[i], nullptr);
    vkFreeMemory(device_->device, uniformMemories_[i], nullptr);
  }
  descriptorMgr_.cleanup(device_->device);
  frameSync_.cleanup(device_->device);
  swapchain_->cleanup();
  swapchain_.reset();
  device_.reset();
}

void RendererContext::createRenderPass() {
  auto device = device_->device;
  VkAttachmentDescription colorAtt{};
  colorAtt.format = swapchain_->getImageFormat();
  colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  // (you'll want a depthAttachment here too)

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;

  VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  rpci.attachmentCount = 1;
  rpci.pAttachments = &colorAtt;
  rpci.subpassCount = 1;
  rpci.pSubpasses = &subpass;

  if (vkCreateRenderPass(device, &rpci, nullptr, &renderPass_) != VK_SUCCESS)
    throw std::runtime_error("Failed to create render pass");
}
