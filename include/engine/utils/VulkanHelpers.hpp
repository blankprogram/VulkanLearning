
#pragma once
#include <vulkan/vulkan.h>

namespace engine::utils {

// Finds a memory type matching the given properties
uint32_t FindMemoryType(VkPhysicalDevice physDevice, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties);

// Creates a device-local buffer by staging `data` through a CPU-visible buffer,
// then copies into `outBuffer`. Frees the staging resources once done.
void CreateBuffer(VkDevice device, VkPhysicalDevice physDevice,
                  VkCommandPool cmdPool, VkQueue queue, const void *data,
                  VkDeviceSize size, VkBufferUsageFlags usage,
                  VkBuffer &outBuffer, VkDeviceMemory &outMemory);

} // namespace engine::utils
