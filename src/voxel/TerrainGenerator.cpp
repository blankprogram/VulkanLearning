
#include "engine/voxel/TerrainGenerator.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace engine {

TerrainData TerrainGenerator::createFlatTerrain(
    int width, int depth, int cubeSize, int brickDim,
    const vk::raii::Device &device, const vk::raii::PhysicalDevice &physical,
    VkDescriptorPool descriptorPool, VkDescriptorSetLayout uboLayout,
    Buffer &vertexBuffer, Buffer &indexBuffer) {
  TerrainData out;

  // 1) build one big VoxelChunk that covers entire terrain
  //    (we'll treat each cube cell as a “voxel” here)
  VoxelChunk chunk(std::max(width, depth));
  for (int z = 0; z < depth; ++z)
    for (int x = 0; x < width; ++x)
      chunk.setVoxel(x, 0, z, true);

  // 2) build octree & SSBO
  out.voxelResources = VoxelResources::create(chunk, brickDim, device, physical,
                                              descriptorPool, uboLayout);

  // 3) build instance‑matrix list (one cube per cell)
  out.instanceCount = width * depth;
  std::vector<glm::mat4> mats;
  mats.reserve(out.instanceCount);
  for (int z = 0; z < depth; ++z) {
    for (int x = 0; x < width; ++x) {
      glm::mat4 M = glm::translate(glm::mat4(1.0f),
                                   glm::vec3(x * cubeSize, 0.0f, z * cubeSize));
      mats.push_back(M);
    }
  }

  // 4) upload instance buffer
  vk::DeviceSize size = sizeof(glm::mat4) * mats.size();
  out.instanceBuffer = std::make_unique<Buffer>(
      physical, device, size, vk::BufferUsageFlagBits::eVertexBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible |
          vk::MemoryPropertyFlagBits::eHostCoherent);
  out.instanceBuffer->copyFrom(mats.data(), size);

  // 5) leave vertexBuffer & indexBuffer alone (reuse your existing cube mesh)

  return out;
}

} // namespace engine
