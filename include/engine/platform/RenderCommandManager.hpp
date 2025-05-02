

#pragma once
#include <vector>
#include <vulkan/vulkan.h>

class RenderCommandManager {
  public:
    void init(VkDevice device, uint32_t queueFamilyIndex,
              size_t framesInFlight);
    void cleanup(VkDevice device);

    VkCommandBuffer get(size_t frameIndex) const;
    VkCommandPool getCommandPool() const;

  private:
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;
};
