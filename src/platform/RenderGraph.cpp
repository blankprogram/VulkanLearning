
#include "engine/platform/RenderGraph.hpp"
#include "engine/platform/RenderCommandManager.hpp"
#include "engine/platform/RenderResources.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>

void RenderGraph::beginFrame(RenderResources &resources,
                             RenderCommandManager &commandManager,
                             size_t frameIndex, const glm::mat4 &viewProj,
                             VkImage swapchainImage) {
  resources.updateUniforms(frameIndex, viewProj);
  commandBuffer_ = commandManager.get(frameIndex);
  swapchainImage_ = swapchainImage; // ← STORE IT

  VkCommandBufferBeginInfo beginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(commandBuffer_, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("Failed to begin command buffer");
  }
  VkClearValue clears[2] = {};
  clears[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};
  clears[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo rpBeginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  rpBeginInfo.renderPass = resources.getRenderPass();
  rpBeginInfo.framebuffer = resources.getFramebuffers()[frameIndex];
  rpBeginInfo.renderArea = {{0, 0}, resources.getSwapchain()->getExtent()};
  rpBeginInfo.clearValueCount = 2;
  rpBeginInfo.pClearValues = clears;
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier.image = swapchainImage;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  vkCmdPipelineBarrier(commandBuffer_, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);
  vkCmdBeginRenderPass(commandBuffer_, &rpBeginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  const auto &pipeline = resources.getPipeline();
  vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline.pipeline);

  uint32_t dynOffset = static_cast<uint32_t>(sizeof(glm::mat4) * frameIndex);
  vkCmdBindDescriptorSets(commandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline.layout, 0, 1,
                          &resources.getDescriptorSets()[0], 1, &dynOffset);

  VkViewport viewport = {
      0.f,
      0.f,
      static_cast<float>(resources.getSwapchain()->getExtent().width),
      static_cast<float>(resources.getSwapchain()->getExtent().height),
      0.f,
      1.f};
  VkRect2D scissor = {{0, 0}, resources.getSwapchain()->getExtent()};

  vkCmdSetViewport(commandBuffer_, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer_, 0, 1, &scissor);
}

void RenderGraph::endFrame() {
  vkCmdEndRenderPass(commandBuffer_);

  // 🔧 Insert image layout transition to PRESENT_SRC_KHR
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier.dstAccessMask = 0;
  barrier.image = swapchainImage_;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  vkCmdPipelineBarrier(commandBuffer_,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  if (vkEndCommandBuffer(commandBuffer_) != VK_SUCCESS) {
    throw std::runtime_error("Failed to end command buffer");
  }
}
