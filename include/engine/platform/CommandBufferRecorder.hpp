
#pragma once

#include <stdexcept>
#include <vulkan/vulkan.h>

class CommandBufferRecorder {
public:
  CommandBufferRecorder(VkCommandBuffer commandBuffer)
      : commandBuffer_(commandBuffer), recording_(false) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer_, &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("Failed to begin recording command buffer!");
    }

    recording_ = true;
  }

  ~CommandBufferRecorder() {
    if (recording_) {
      vkEndCommandBuffer(commandBuffer_);
    }
  }

private:
  VkCommandBuffer commandBuffer_;
  bool recording_;
};
