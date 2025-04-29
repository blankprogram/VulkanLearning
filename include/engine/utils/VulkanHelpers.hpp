
// include/engine/utils/VulkanHelpers.hpp
#pragma once

#include <vulkan/vulkan.h>

namespace engine::utils {
// Create a device-local buffer, upload via staging, etc.
// (You can flesh this out or leave it stubbed for now.)
void CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                  const void *data, VkDeviceSize size, VkBufferUsageFlags usage,
                  VkBuffer &outBuffer, VkDeviceMemory &outMemory);
} // namespace engine::utils
