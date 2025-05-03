
// include/engine/memory/DeviceMemory.hpp
#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace engine {

class DeviceMemory {
public:
  DeviceMemory(const vk::raii::Device &device,
               const vk::PhysicalDeviceMemoryProperties &memProps,
               const vk::MemoryRequirements &memRequirements,
               vk::MemoryPropertyFlags properties);

  ~DeviceMemory() = default;

  void *map();
  void unmap();

  vk::DeviceSize size() const { return size_; }
  const vk::raii::DeviceMemory &get() const { return memory_; }

private:
  const vk::raii::Device &device_;
  vk::DeviceSize size_;
  uint32_t memoryTypeIndex_;
  vk::raii::DeviceMemory memory_;
};

} // namespace engine
