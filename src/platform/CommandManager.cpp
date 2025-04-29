
#include "engine/platform/CommandManager.hpp"
#include <stdexcept>

void CommandManager::init(VkDevice device, uint32_t queueFamilyIndex) {
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = queueFamilyIndex;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool_) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create command pool!");
  }
}

void CommandManager::cleanup(VkDevice device) {
  if (commandPool_ != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device, commandPool_, nullptr);
    commandPool_ = VK_NULL_HANDLE;
  }
}
