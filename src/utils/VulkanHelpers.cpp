
#include "engine/utils/VulkanHelpers.hpp"

namespace engine::utils {

void CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                  const void *data, VkDeviceSize size, VkBufferUsageFlags usage,
                  VkBuffer &outBuffer, VkDeviceMemory &outMemory) {
  // stub: you can implement real staging upload here.
  // For now, just zero them so linkage succeeds:
  outBuffer = VK_NULL_HANDLE;
  outMemory = VK_NULL_HANDLE;
}

} // namespace engine::utils
