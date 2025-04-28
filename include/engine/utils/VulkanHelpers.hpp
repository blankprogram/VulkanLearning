
#pragma once

#include <cstdlib>
#include <iostream>
#include <vulkan/vulkan.h>

// Fatal if Vulkan result not VK_SUCCESS
inline void CHECK_VK_RESULT(VkResult result) {
  if (result != VK_SUCCESS) {
    std::cerr << "Vulkan error!" << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                  VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer &buffer,
                  VkDeviceMemory &bufferMemory);
// BEGIN updated signatures

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties);

VkCommandBuffer beginSingleTimeCommands(VkDevice device,
                                        VkCommandPool commandPool);

void endSingleTimeCommands(VkDevice device, VkQueue queue,
                           VkCommandPool commandPool,
                           VkCommandBuffer commandBuffer);

// END updated signatures
