#pragma once
#include <stdexcept>
#include <vulkan/vulkan.h>

class CommandManager {
  public:
    CommandManager(VkDevice device, uint32_t queueFamilyIndex)
        : device_(device) {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndex;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) !=
            VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool!");
        }
    }

    ~CommandManager() {
        if (commandPool_ != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, commandPool_, nullptr);
        }
    }

    CommandManager(const CommandManager &) = delete;
    CommandManager &operator=(const CommandManager &) = delete;

    CommandManager(CommandManager &&other) noexcept
        : device_(other.device_), commandPool_(other.commandPool_) {
        other.commandPool_ = VK_NULL_HANDLE;
    }
    CommandManager &operator=(CommandManager &&other) noexcept {
        if (this != &other) {
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
