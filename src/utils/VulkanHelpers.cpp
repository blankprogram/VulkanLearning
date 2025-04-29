#include "engine/utils/VulkanHelpers.hpp"
#include <cstring>
#include <stdexcept>

namespace engine::utils {

uint32_t FindMemoryType(VkPhysicalDevice physDevice, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProps;
  vkGetPhysicalDeviceMemoryProperties(physDevice, &memProps);
  for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
    if ((typeFilter & (1 << i)) &&
        (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  throw std::runtime_error("Failed to find suitable memory type");
}

void CreateBuffer(VkDevice device, VkPhysicalDevice physDevice,
                  VkCommandPool cmdPool, VkQueue queue, const void *data,
                  VkDeviceSize size, VkBufferUsageFlags usage,
                  VkBuffer &outBuffer, VkDeviceMemory &outMemory) {
  // 1) Create staging buffer (HOST_VISIBLE | COHERENT)
  VkBuffer stagingBuf;
  VkDeviceMemory stagingMem;
  VkBufferCreateInfo bufInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufInfo.size = size;
  bufInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkCreateBuffer(device, &bufInfo, nullptr, &stagingBuf);

  VkMemoryRequirements memReq;
  vkGetBufferMemoryRequirements(device, stagingBuf, &memReq);
  VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocInfo.allocationSize = memReq.size;
  allocInfo.memoryTypeIndex =
      FindMemoryType(physDevice, memReq.memoryTypeBits,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                         VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  vkAllocateMemory(device, &allocInfo, nullptr, &stagingMem);
  vkBindBufferMemory(device, stagingBuf, stagingMem, 0);

  // Copy data into staging
  void *mapped;
  vkMapMemory(device, stagingMem, 0, size, 0, &mapped);
  std::memcpy(mapped, data, (size_t)size);
  vkUnmapMemory(device, stagingMem);

  // 2) Create device-local buffer
  VkBufferCreateInfo devBufInfo = bufInfo;
  devBufInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  vkCreateBuffer(device, &devBufInfo, nullptr, &outBuffer);

  vkGetBufferMemoryRequirements(device, outBuffer, &memReq);
  allocInfo.allocationSize = memReq.size;
  allocInfo.memoryTypeIndex = FindMemoryType(
      physDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  vkAllocateMemory(device, &allocInfo, nullptr, &outMemory);
  vkBindBufferMemory(device, outBuffer, outMemory, 0);

  // 3) Copy from staging to device-local
  VkCommandBufferAllocateInfo cmdAlloc{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmdAlloc.commandPool = cmdPool;
  cmdAlloc.commandBufferCount = 1;
  VkCommandBuffer cmd;
  vkAllocateCommandBuffers(device, &cmdAlloc, &cmd);

  VkCommandBufferBeginInfo beginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd, &beginInfo);

  VkBufferCopy copyRegion{0, 0, size};
  vkCmdCopyBuffer(cmd, stagingBuf, outBuffer, 1, &copyRegion);
  vkEndCommandBuffer(cmd);

  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &cmd;
  vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  // Cleanup staging
  vkFreeCommandBuffers(device, cmdPool, 1, &cmd);
  vkDestroyBuffer(device, stagingBuf, nullptr);
  vkFreeMemory(device, stagingMem, nullptr);
}

} // namespace engine::utils
