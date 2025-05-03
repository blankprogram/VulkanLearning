#include "engine/memory/DeviceMemory.hpp"
#include <stdexcept>

namespace engine {

static uint32_t
findMemoryType(const vk::PhysicalDeviceMemoryProperties &memProps,
               uint32_t typeFilter, vk::MemoryPropertyFlags props) {
  for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
    if ((typeFilter & (1u << i)) &&
        (memProps.memoryTypes[i].propertyFlags & props) == props) {
      return i;
    }
  }
  throw std::runtime_error("DeviceMemory: no suitable memory type found");
}

DeviceMemory::DeviceMemory(const vk::raii::Device &device,
                           const vk::PhysicalDeviceMemoryProperties &memProps,
                           const vk::MemoryRequirements &memRequirements,
                           vk::MemoryPropertyFlags properties)
    : device_{device}, size_{memRequirements.size},
      memoryTypeIndex_{
          findMemoryType(memProps, memRequirements.memoryTypeBits, properties)},
      memory_{device, vk::MemoryAllocateInfo{}
                          .setAllocationSize(memRequirements.size)
                          .setMemoryTypeIndex(memoryTypeIndex_)} {}

void *DeviceMemory::map() { return memory_.mapMemory(0, size_); }

void DeviceMemory::unmap() { memory_.unmapMemory(); }

} // namespace engine
