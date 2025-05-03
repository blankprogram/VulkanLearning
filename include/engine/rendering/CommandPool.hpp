#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace engine {

class CommandPool {
public:
  CommandPool(const vk::raii::Device &device, uint32_t queueFamilyIndex,
              vk::CommandPoolCreateFlags flags =
                  vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

  ~CommandPool() = default;
  CommandPool(const CommandPool &) = delete;
  CommandPool &operator=(const CommandPool &) = delete;
  CommandPool(CommandPool &&) = default;
  CommandPool &operator=(CommandPool &&) = delete;

  vk::raii::CommandBuffers allocate(
      uint32_t count,
      vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;

  const vk::raii::CommandPool &get() const { return pool_; }

private:
  const vk::raii::Device &device_;
  vk::raii::CommandPool pool_;
};

} // namespace engine
