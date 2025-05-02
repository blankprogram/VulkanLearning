
#pragma once
#include <vulkan/vulkan.h>

namespace engine::utils {

uint32_t FindMemoryType(VkPhysicalDevice physDevice, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties);

void CreateBuffer(VkDevice device, VkPhysicalDevice physDevice,
                  VkCommandPool cmdPool, VkQueue queue, const void *data,
                  VkDeviceSize size, VkBufferUsageFlags usage,
                  VkBuffer &outBuffer, VkDeviceMemory &outMemory);

void CreateHostVisibleBuffer(VkDevice device, VkPhysicalDevice physDevice,
                             VkDeviceSize size, VkBufferUsageFlags usage,
                             VkBuffer &outBuffer, VkDeviceMemory &outMemory);

} // namespace engine::utils
