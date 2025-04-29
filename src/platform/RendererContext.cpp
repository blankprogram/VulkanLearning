
// src/platform/RendererContext.cpp
#include "engine/platform/RendererContext.hpp"
#include "engine/platform/DescriptorManager.hpp"
#include "engine/utils/VulkanHelpers.hpp"
#include <array>
#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

static constexpr uint32_t WIDTH = 1280;
static constexpr uint32_t HEIGHT = 720;

RendererContext::RendererContext(GLFWwindow *window)
    : cam_(glm::radians(45.0f), float(WIDTH) / float(HEIGHT), 0.1f, 100.0f) {
  init(window);
}

RendererContext::~RendererContext() { cleanup(); }

void RendererContext::init(GLFWwindow *window) {
  // 1) device, swapchain, sync objects
  device_ = std::make_unique<VulkanDevice>(window);
  swapchain_ =
      std::make_unique<Swapchain>(device_.get(), device_->surface, window);
  frameSync_.init(device_->device, MAX_FRAMES_IN_FLIGHT);
  VmaAllocatorCreateInfo ai{};
  ai.physicalDevice = device_->physicalDevice;
  ai.device = device_->device;
  ai.instance = device_->instance;
  vmaCreateAllocator(&ai, &allocator_);
  extent_ = swapchain_->getExtent();

  createDepthResources();
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

  // — Allocate one big HOST_VISIBLE, coherent UBO for all frames via VMA —
  VkDeviceSize totalSize = sizeof(glm::mat4) * MAX_FRAMES_IN_FLIGHT;
  VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = totalSize;
  bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
  allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  // allocator_ must have been created in init()
  vmaCreateBuffer(allocator_, &bufferInfo, &allocInfo, &uniformBuffer_,
                  &uniformAllocation_, nullptr);

  // — Descriptor: one set, dynamic UBO —
  descriptorMgr_.init(dev, 1);

  VkDescriptorBufferInfo dbi{};
  dbi.buffer = uniformBuffer_;
  dbi.offset = 0;
  dbi.range = sizeof(glm::mat4);

  VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  write.dstSet = descriptorMgr_.getDescriptorSets()[0];
  write.dstBinding = 0;
  write.dstArrayElement = 0;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  write.pBufferInfo = &dbi;

  vkUpdateDescriptorSets(dev, 1, &write, 0, nullptr);
}

void RendererContext::beginFrame() {
  // 1) Wait for last frame’s fence and acquire the next swapchain image
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

  // 2) Update our per-frame UBO via VMA
  glm::mat4 viewProj = cam_.viewProjection();
  void *mapped;
  vmaMapMemory(allocator_, uniformAllocation_, &mapped);
  std::memcpy(reinterpret_cast<char *>(mapped) +
                  sizeof(glm::mat4) * currentFrame_,
              &viewProj, sizeof(viewProj));
  vmaUnmapMemory(allocator_, uniformAllocation_);

  // 3) Begin recording commands into today's command buffer
  currentCommandBuffer_ = commandBuffers_[currentFrame_];
  VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(currentCommandBuffer_, &bi);

  // 4) Begin render pass with both color and depth clears
  VkClearValue clearValues[2];
  // clear color attachment (attachment 0)
  clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
  // clear depth attachment (attachment 1) to far plane = 1.0
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  rpbi.renderPass = renderPass_;
  rpbi.framebuffer = framebuffers_[currentImageIndex_];
  rpbi.renderArea = {{0, 0}, extent_};
  rpbi.clearValueCount = 2;
  rpbi.pClearValues = clearValues;
  vkCmdBeginRenderPass(currentCommandBuffer_, &rpbi,
                       VK_SUBPASS_CONTENTS_INLINE);

  // 5) Bind pipeline and descriptor set with dynamic offset
  vkCmdBindPipeline(currentCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline_.pipeline);
  uint32_t dynamicOffset =
      static_cast<uint32_t>(sizeof(glm::mat4) * currentFrame_);
  vkCmdBindDescriptorSets(
      currentCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.layout,
      0, 1, &descriptorMgr_.getDescriptorSets()[0], 1, &dynamicOffset);

  // 6) Set viewport & scissor
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
  // destroy old ones if any
  for (auto fb : framebuffers_)
    vkDestroyFramebuffer(device_->device, fb, nullptr);
  framebuffers_.clear();

  auto &colorViews = swapchain_->getImageViews();
  framebuffers_.resize(colorViews.size());

  for (size_t i = 0; i < colorViews.size(); ++i) {
    VkImageView attachments[] = {colorViews[i], depthImageView_};

    VkFramebufferCreateInfo fci{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fci.renderPass = renderPass_;
    fci.attachmentCount = 2;
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
  createDepthResources();
  createRenderPass();
  createFramebuffers();
  pipeline_.init(device_->device, renderPass_, descriptorMgr_.getLayout(),
                 std::string(SPIRV_OUT) + "/vert.spv",
                 std::string(SPIRV_OUT) + "/frag.spv");
}

void RendererContext::cleanup() {
  // 1) Destroy framebuffers
  for (auto fb : framebuffers_)
    vkDestroyFramebuffer(device_->device, fb, nullptr);
  framebuffers_.clear();

  // 2) Destroy render pass
  if (renderPass_ != VK_NULL_HANDLE)
    vkDestroyRenderPass(device_->device, renderPass_, nullptr);

  // 3) Destroy depth view & image
  if (depthImageView_ != VK_NULL_HANDLE)
    vkDestroyImageView(device_->device, depthImageView_, nullptr);
  if (depthImage_ != VK_NULL_HANDLE)
    vmaDestroyImage(allocator_, depthImage_, depthImageAllocation_);

  // 4) Cleanup swapchain
  swapchain_->cleanup();

  // 5) Destroy allocator’s UBO & allocator itself, then device
  vmaDestroyBuffer(allocator_, uniformBuffer_, uniformAllocation_);
  vmaDestroyAllocator(allocator_);

  descriptorMgr_.cleanup(device_->device);
  frameSync_.cleanup(device_->device);
  device_.reset();
}

void RendererContext::createRenderPass() {
  // — 1) Color attachment —
  VkAttachmentDescription colorAtt{};
  colorAtt.format = swapchain_->getImageFormat();
  colorAtt.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAtt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAtt.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  // — 2) Depth attachment —
  VkAttachmentDescription depthAtt{};
  depthAtt.format = depthFormat_;
  depthAtt.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAtt.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAtt.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAtt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAtt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAtt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAtt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  std::array<VkAttachmentDescription, 2> atts = {colorAtt, depthAtt};

  // references for subpass
  VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkAttachmentReference depthRef{
      1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;
  subpass.pDepthStencilAttachment = &depthRef;

  VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  rpci.attachmentCount = static_cast<uint32_t>(atts.size());
  rpci.pAttachments = atts.data();
  rpci.subpassCount = 1;
  rpci.pSubpasses = &subpass;

  if (vkCreateRenderPass(device_->device, &rpci, nullptr, &renderPass_) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to create render pass");
}
VkFormat RendererContext::findDepthFormat() const {
  // candidates in order of preference:
  std::vector<VkFormat> candidates = {VK_FORMAT_D32_SFLOAT,
                                      VK_FORMAT_D32_SFLOAT_S8_UINT,
                                      VK_FORMAT_D24_UNORM_S8_UINT};

  for (auto fmt : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(device_->physicalDevice, fmt, &props);
    if (props.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
      return fmt;
  }

  throw std::runtime_error("Failed to find supported depth format");
}

void RendererContext::createDepthResources() {
  depthFormat_ = findDepthFormat();

  // 1) Create the VkImage
  VkImageCreateInfo imgInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imgInfo.imageType = VK_IMAGE_TYPE_2D;
  imgInfo.extent.width = extent_.width;
  imgInfo.extent.height = extent_.height;
  imgInfo.extent.depth = 1;
  imgInfo.mipLevels = 1;
  imgInfo.arrayLayers = 1;
  imgInfo.format = depthFormat_;
  imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  if (vmaCreateImage(allocator_, &imgInfo, &allocInfo, &depthImage_,
                     &depthImageAllocation_, nullptr) != VK_SUCCESS)
    throw std::runtime_error("Failed to allocate depth image");

  // 2) Create the VkImageView
  VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  viewInfo.image = depthImage_;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = depthFormat_;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device_->device, &viewInfo, nullptr,
                        &depthImageView_) != VK_SUCCESS)
    throw std::runtime_error("Failed to create depth image view");
}
