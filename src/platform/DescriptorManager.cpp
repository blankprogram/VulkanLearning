#include "engine/platform/DescriptorManager.hpp"
#include <stdexcept>

void DescriptorManager::init(VkDevice device) {
  // Example: basic UBO layout
  VkDescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  uboLayoutBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &uboLayoutBinding;

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                  &descriptorSetLayout_) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor set layout!");
  }

  // Create a simple pool (adjust sizes as needed)
  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = 100;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = 100;

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool_) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor pool!");
  }
}

void DescriptorManager::cleanup(VkDevice device) {
  if (descriptorPool_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device, descriptorPool_, nullptr);
    descriptorPool_ = VK_NULL_HANDLE;
  }
  if (descriptorSetLayout_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout_, nullptr);
    descriptorSetLayout_ = VK_NULL_HANDLE;
  }
}
