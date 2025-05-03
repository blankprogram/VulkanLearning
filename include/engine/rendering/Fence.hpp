#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace engine {

class Fence {
public:
  explicit Fence(const vk::raii::Device &device, bool signaled = false);
  ~Fence() = default;
  Fence(const Fence &) = delete;
  Fence &operator=(const Fence &) = delete;
  Fence(Fence &&) = default;
  Fence &operator=(Fence &&) = delete;

  VkFence get() const { return *fence_; }

  void reset();

private:
  vk::raii::Fence fence_;
  const vk::raii::Device &device_;
};

} // namespace engine
