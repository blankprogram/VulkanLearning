#include "engine/platform/RenderCommandManager.hpp"
#include <stdexcept>

void RenderCommandManager::init(VkDevice device, uint32_t queueFamilyIndex,
                                size_t frameCount) {
    VkCommandPoolCreateInfo poolInfo{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = queueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool_) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }

    VkCommandBufferAllocateInfo allocInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(frameCount);

    commandBuffers_.resize(frameCount);
    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers_.data()) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }
}

void RenderCommandManager::cleanup(VkDevice device) {
    if (commandPool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, commandPool_, nullptr);
        commandPool_ = VK_NULL_HANDLE;
    }
}

VkCommandBuffer RenderCommandManager::get(size_t frameIndex) const {
    return commandBuffers_.at(frameIndex);
}

VkCommandPool RenderCommandManager::getCommandPool() const {
    return commandPool_;
}
