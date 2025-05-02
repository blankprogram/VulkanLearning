

#pragma once
#include <vector>
#include <vulkan/vulkan.h>

class DescriptorManager {
  public:
    void init(VkDevice device, uint32_t maxFrames);
    void cleanup(VkDevice device);

    VkDescriptorSetLayout getLayout() const { return layout_; }
    const std::vector<VkDescriptorSet> &getDescriptorSets() const {
        return sets_;
    }

  private:
    VkDescriptorSetLayout layout_{VK_NULL_HANDLE};
    VkDescriptorPool pool_{VK_NULL_HANDLE};
    std::vector<VkDescriptorSet> sets_;
};
