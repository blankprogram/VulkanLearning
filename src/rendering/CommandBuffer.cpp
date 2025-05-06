#include "engine/rendering/CommandBuffer.hpp"
#include <stdexcept>

namespace engine {

static vk::raii::CommandBuffer
makeSingleBuffer(const vk::raii::Device &device,
                 const vk::raii::CommandPool &pool,
                 vk::CommandBufferLevel level) {
  vk::CommandBufferAllocateInfo allocInfo{};
  allocInfo.setCommandPool(*pool).setLevel(level).setCommandBufferCount(1);

  vk::raii::CommandBuffers tmp{device, allocInfo};
  if (tmp.empty()) {
    throw std::runtime_error("Failed to allocate command buffer");
  }
  return std::move(tmp.front());
}

CommandBuffer::CommandBuffer(const vk::raii::Device &device,
                             const vk::raii::CommandPool &pool,
                             vk::CommandBufferLevel level)
    : buffer_{makeSingleBuffer(device, pool, level)} {}

void CommandBuffer::begin(vk::CommandBufferUsageFlags flags) {
  vk::CommandBufferBeginInfo info{};
  info.setFlags(flags);
  buffer_.begin(info);
}

void CommandBuffer::end() { buffer_.end(); }

} // namespace engine
