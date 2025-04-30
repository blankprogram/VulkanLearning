#include "engine/platform/UniformManager.hpp"
#include <cstring>
#include <stdexcept>

void UniformManager::init(VkDevice device, VmaAllocator allocator,
                          size_t frameCount) {
  VkDeviceSize bufferSize = sizeof(glm::mat4) * frameCount;
  VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bufferInfo.size = bufferSize;
  bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

  VmaAllocationCreateInfo allocInfo{};
  allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
  allocInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer_, &allocation_,
                  nullptr);

  descriptorMgr_.init(device, 1);

  VkDescriptorBufferInfo dbi{};
  dbi.buffer = buffer_;
  dbi.offset = 0;
  dbi.range = sizeof(glm::mat4);

  VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  write.dstSet = descriptorMgr_.getDescriptorSets()[0];
  write.dstBinding = 0;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  write.pBufferInfo = &dbi;

  vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}

void UniformManager::update(VmaAllocator allocator, size_t frameIndex,
                            const glm::mat4 &mat) {
  void *mapped;
  vmaMapMemory(allocator, allocation_, &mapped);
  std::memcpy(static_cast<char *>(mapped) + sizeof(glm::mat4) * frameIndex,
              &mat, sizeof(mat));
  vmaUnmapMemory(allocator, allocation_);
}

void UniformManager::cleanup(VkDevice device, VmaAllocator allocator) {
  descriptorMgr_.cleanup(device);
  vmaDestroyBuffer(allocator, buffer_, allocation_);
  buffer_ = VK_NULL_HANDLE;
  allocation_ = VK_NULL_HANDLE;
}
