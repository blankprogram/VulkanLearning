#include "engine/memory/Buffer.hpp"
#include <cstring>
#include <stdexcept>

namespace engine {

Buffer::Buffer(const vk::raii::PhysicalDevice &physical,
               const vk::raii::Device &device, vk::DeviceSize size,
               vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
    : size_{size},
      buffer_{
          device,
          vk::BufferCreateInfo{}.setSize(size).setUsage(usage).setSharingMode(
              vk::SharingMode::eExclusive)},
      memory_{device, physical.getMemoryProperties(),
              buffer_.getMemoryRequirements(), properties} {
  buffer_.bindMemory(*memory_.get(), 0);
}

void Buffer::copyFrom(const void *srcData, vk::DeviceSize length,
                      vk::DeviceSize offset) {
  if (offset + length > size_) {
    throw std::runtime_error("Buffer::copyFrom out of bounds");
  }
  void *dst = map();
  std::memcpy(static_cast<char *>(dst) + offset, srcData,
              static_cast<size_t>(length));
  unmap();
}

} // namespace engine
