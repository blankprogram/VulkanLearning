
// include/engine/platform/CommandManager.hpp
#pragma once
#include <stdexcept>
#include <vulkan/vulkan.h>

// RAII wrapper for a Vulkan command pool
class CommandManager {
public:
  // Construct and immediately create a command pool for the given device and
  // queue family
  CommandManager(VkDevice device, uint32_t queueFamilyIndex) : device_(device) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) !=
        VK_SUCCESS) {
      throw std::runtime_error("Failed to create command pool!");
    }
  }

  // Destroy the command pool when this object goes out of scope
  ~CommandManager() {
    if (commandPool_ != VK_NULL_HANDLE) {
      vkDestroyCommandPool(device_, commandPool_, nullptr);
    }
  }

  // Non-copyable
  CommandManager(const CommandManager &) = delete;
  CommandManager &operator=(const CommandManager &) = delete;

  // Movable
  CommandManager(CommandManager &&other) noexcept
      : device_(other.device_), commandPool_(other.commandPool_) {
    other.commandPool_ = VK_NULL_HANDLE;
  }
  CommandManager &operator=(CommandManager &&other) noexcept {
    if (this != &other) {
      // destroy existing pool
      if (commandPool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device_, commandPool_, nullptr);
      }
      device_ = other.device_;
      commandPool_ = other.commandPool_;
      other.commandPool_ = VK_NULL_HANDLE;
    }
    return *this;
  }

  VkCommandPool getCommandPool() const { return commandPool_; }

private:
  VkDevice device_{VK_NULL_HANDLE};
  VkCommandPool commandPool_{VK_NULL_HANDLE};
};
