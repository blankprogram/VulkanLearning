
#pragma once

#include "engine/memory/Buffer.hpp"
#include "engine/voxel/VoxelChunk.hpp"
#include "engine/voxel/VoxelResources.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

struct TerrainData {
  // the TH‑octree SSBO & descriptor
  VoxelResources voxelResources;
  // per‑instance model‑matrices buffer
  std::unique_ptr<Buffer> instanceBuffer;
  uint32_t instanceCount = 0;
};

class TerrainGenerator {
public:
  /// Generate a flat terrain of size width×depth (in cubes), at y=0.
  /// brickDim is passed through to the VoxelChunk octree builder.
  static TerrainData createFlatTerrain(
      int width, int depth, int cubeSize, int brickDim,
      const vk::raii::Device &device, const vk::raii::PhysicalDevice &physical,
      VkDescriptorPool descriptorPool, VkDescriptorSetLayout uboLayout,
      Buffer &vertexBuffer, Buffer &indexBuffer);
};

} // namespace engine
