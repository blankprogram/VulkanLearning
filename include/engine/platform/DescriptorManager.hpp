
#pragma once
#include <vulkan/vulkan.h>

class DescriptorManager {
public:
  void init(VkDevice device);
  void cleanup(VkDevice device);

  VkDescriptorSetLayout getLayout() const { return descriptorSetLayout_; }
  VkDescriptorPool getPool() const { return descriptorPool_; }

private:
  VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
  VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
};
