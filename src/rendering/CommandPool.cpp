#include "engine/rendering/CommandPool.hpp"

namespace engine {

static vk::raii::CommandPool makePool(const vk::raii::Device &device,
                                      uint32_t queueFamilyIndex,
                                      vk::CommandPoolCreateFlags flags) {
  vk::CommandPoolCreateInfo info{};
  info.setQueueFamilyIndex(queueFamilyIndex).setFlags(flags);
  return vk::raii::CommandPool{device, info};
}

CommandPool::CommandPool(const vk::raii::Device &device,
                         uint32_t queueFamilyIndex,
                         vk::CommandPoolCreateFlags flags)
    : device_{device}, pool_{makePool(device, queueFamilyIndex, flags)} {}

vk::raii::CommandBuffers
CommandPool::allocate(uint32_t count, vk::CommandBufferLevel level) const {
  vk::CommandBufferAllocateInfo allocInfo{};
  allocInfo.setCommandPool(*pool_).setLevel(level).setCommandBufferCount(count);

  return vk::raii::CommandBuffers{device_, allocInfo};
}

} // namespace engine
