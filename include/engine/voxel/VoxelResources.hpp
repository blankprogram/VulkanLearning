#pragma once

#include "engine/memory/Buffer.hpp"
#include "engine/scene/VoxelChunk.hpp"
#include "engine/voxel/VoxelChunk.hpp"
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

struct VoxelResources {
  std::vector<THNode> nodes;
  std::unique_ptr<Buffer> buffer;
  std::unique_ptr<vk::raii::DescriptorSetLayout> layout;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

  static VoxelResources create(const VoxelChunk &chunk, int brickDim,
                               const vk::raii::Device &device,
                               const vk::raii::PhysicalDevice &physical,
                               VkDescriptorPool descriptorPool,
                               VkDescriptorSetLayout uboLayout);
};

} // namespace engine
