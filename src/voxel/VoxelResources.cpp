#include "engine/voxel/VoxelResources.hpp"
#include <stdexcept>

namespace engine {

VoxelResources VoxelResources::create(const VoxelChunk &chunk, int brickDim,
                                      const vk::raii::Device &device,
                                      const vk::raii::PhysicalDevice &physical,
                                      VkDescriptorPool descriptorPool,
                                      VkDescriptorSetLayout uboLayout) {
  VoxelResources R;

  // --- build the TH‑octree on CPU ---
  R.nodes = chunk.buildTHOctree(brickDim);
  size_t byteCount = R.nodes.size() * sizeof(THNode);

  // --- allocate & fill host‑visible SSBO ---
  R.buffer = std::make_unique<Buffer>(
      physical, device, byteCount, vk::BufferUsageFlagBits::eStorageBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible |
          vk::MemoryPropertyFlagBits::eHostCoherent);
  R.buffer->copyFrom(R.nodes.data(), byteCount);

  // --- create a descriptor‑set‑layout for binding=1 (the SSBO) ---
  vk::DescriptorSetLayoutBinding binding{};
  binding.binding = 1;
  binding.descriptorType = vk::DescriptorType::eStorageBuffer;
  binding.descriptorCount = 1;
  binding.stageFlags =
      vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;

  R.layout = std::make_unique<vk::raii::DescriptorSetLayout>(
      device,
      vk::DescriptorSetLayoutCreateInfo{}.setBindingCount(1).setPBindings(
          &binding));

  VkDescriptorSetLayout rawLayouts[2] = {
      uboLayout, static_cast<VkDescriptorSetLayout>(**R.layout)};

  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = 2;
  allocInfo.pSetLayouts = rawLayouts;

  VkDescriptorSet rawSets[2];
  if (vkAllocateDescriptorSets(static_cast<VkDevice>(*device), &allocInfo,
                               rawSets) != VK_SUCCESS) {
    throw std::runtime_error("failed to allocate voxel descriptor set");
  }
  R.descriptorSet = rawSets[1];

  // --- write the SSBO binding into that set ---
  VkDescriptorBufferInfo bufInfo{};
  bufInfo.buffer = R.buffer->get();
  bufInfo.offset = 0;
  bufInfo.range = byteCount;

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet = R.descriptorSet;
  write.dstBinding = 1;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  write.pBufferInfo = &bufInfo;
  vkUpdateDescriptorSets(static_cast<VkDevice>(*device), 1, &write, 0, nullptr);

  return R;
}

} // namespace engine
