
#pragma once
#include "engine/platform/DescriptorManager.hpp"
#include "externals/vk_mem_alloc.h"
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

class UniformManager {
  public:
    void init(VkDevice device, VmaAllocator allocator, size_t frameCount);
    void update(VmaAllocator allocator, size_t frameIndex,
                const glm::mat4 &mat);
    void cleanup(VkDevice device, VmaAllocator allocator);

    VkBuffer buffer() const { return buffer_; }
    VkDescriptorSetLayout layout() const { return descriptorMgr_.getLayout(); }
    const std::vector<VkDescriptorSet> &sets() const {
        return descriptorMgr_.getDescriptorSets();
    }

  private:
    DescriptorManager descriptorMgr_;
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = VK_NULL_HANDLE;
};
