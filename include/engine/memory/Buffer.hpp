#pragma once

#include "DeviceMemory.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class Buffer {
public:
  Buffer(const vk::raii::PhysicalDevice &physical,
         const vk::raii::Device &device, vk::DeviceSize size,
         vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);

  ~Buffer() = default;

  vk::Buffer get() const { return *buffer_; }
  const vk::raii::Buffer &raw() const { return buffer_; }

  void *map() { return memory_.map(); }
  void unmap() { memory_.unmap(); }

  void copyFrom(const void *srcData, vk::DeviceSize length,
                vk::DeviceSize offset = 0);

  vk::DeviceSize size() const { return size_; }

private:
  vk::DeviceSize size_;
  vk::raii::Buffer buffer_;
  DeviceMemory memory_;
};

} // namespace engine
