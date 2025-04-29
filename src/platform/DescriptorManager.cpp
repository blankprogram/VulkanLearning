
#include "engine/platform/DescriptorManager.hpp"
#include <stdexcept>

void DescriptorManager::init(VkDevice device, uint32_t maxFrames) {
  // 1) create layout
  VkDescriptorSetLayoutBinding b{};
  b.binding = 0;
  b.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  b.descriptorCount = 1;
  b.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  VkDescriptorSetLayoutCreateInfo li{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  li.bindingCount = 1;
  li.pBindings = &b;
  if (vkCreateDescriptorSetLayout(device, &li, nullptr, &layout_) != VK_SUCCESS)
    throw std::runtime_error("Failed to create descriptor set layout");

  // 2) pool
  VkDescriptorPoolSize ps{};
  ps.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  ps.descriptorCount = maxFrames;
  VkDescriptorPoolCreateInfo pi{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  pi.poolSizeCount = 1;
  pi.pPoolSizes = &ps;
  pi.maxSets = maxFrames;
  if (vkCreateDescriptorPool(device, &pi, nullptr, &pool_) != VK_SUCCESS)
    throw std::runtime_error("Failed to create descriptor pool");

  // 3) allocate sets
  sets_.resize(maxFrames);
  std::vector<VkDescriptorSetLayout> layouts(maxFrames, layout_);
  VkDescriptorSetAllocateInfo ai{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  ai.descriptorPool = pool_;
  ai.descriptorSetCount = maxFrames;
  ai.pSetLayouts = layouts.data();
  if (vkAllocateDescriptorSets(device, &ai, sets_.data()) != VK_SUCCESS)
    throw std::runtime_error("Failed to allocate descriptor sets");
}

void DescriptorManager::cleanup(VkDevice device) {
  if (pool_)
    vkDestroyDescriptorPool(device, pool_, nullptr);
  if (layout_)
    vkDestroyDescriptorSetLayout(device, layout_, nullptr);
}
