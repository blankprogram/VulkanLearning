#ifndef ENGINE_UTILS_VULKANHELPERS_HPP
#define ENGINE_UTILS_VULKANHELPERS_HPP

#include <cstdlib>
#include <iostream>
#include <vulkan/vulkan.h>

// Fatal if Vulkan result not VK_SUCCESS
inline void CHECK_VK_RESULT(VkResult result) {
  if (result != VK_SUCCESS) {
    std::cerr << "Vulkan error at " << __FILE__ << ":" << __LINE__ << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

// Allocate and bind a buffer of given size/usage/memory
void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                  VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer &buffer,
                  VkDeviceMemory &bufferMemory);

void copyBuffer(VkDevice device, VkCommandPool pool, VkQueue graphicsQueue,
                VkBuffer src, VkBuffer dst, VkDeviceSize size);

// Find memory type on GPU matching requirements
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties);

// Begin a one-time command buffer
VkCommandBuffer beginSingleTimeCommands(VkDevice device,
                                        VkCommandPool commandPool);

// End and submit a one-time command buffer
void endSingleTimeCommands(VkDevice device, VkQueue graphicsQueue,
                           VkCommandPool commandPool,
                           VkCommandBuffer commandBuffer);

#endif // ENGINE_UTILS_VULKANHELPERS_HPP
