#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace engine {

class CommandBuffer {
public:
  CommandBuffer(
      const vk::raii::Device &device, const vk::raii::CommandPool &pool,
      vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);

  ~CommandBuffer() = default;
  CommandBuffer(const CommandBuffer &) = delete;
  CommandBuffer &operator=(const CommandBuffer &) = delete;
  CommandBuffer(CommandBuffer &&) = default;
  CommandBuffer &operator=(CommandBuffer &&) = default;

  void begin(vk::CommandBufferUsageFlags flags =
                 vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
  void end();

  vk::CommandBuffer get() const { return *buffer_; }

private:
  vk::raii::CommandBuffer buffer_;
};

} // namespace engine
