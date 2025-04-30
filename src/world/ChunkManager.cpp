#include "engine/world/ChunkManager.hpp"
#include "engine/voxel/VoxelMesher.hpp"
#include "engine/world/TerrainGenerator.hpp"
#include <cmath> // std::floor
#include <glm/glm.hpp>
#include <iostream>

using namespace engine::world;
using engine::voxel::VoxelMesher;

ChunkManager::ChunkManager() = default;
void ChunkManager::initChunks(engine::utils::ThreadPool &threadPool) {
  // generate initial chunks around origin
  updateChunks(glm::vec3{0, 0, 0}, threadPool);
}
void ChunkManager::updateChunks(const glm::vec3 &playerPos,
                                engine::utils::ThreadPool &threadPool) {
  // compute player chunk

  glm::ivec3 pChunk = glm::ivec3(glm::floor(playerPos / glm::vec3(CHUNK_DIM)));

  for (int dz = -viewRadius_; dz <= viewRadius_; ++dz)
    for (int dy = -viewRadius_; dy <= viewRadius_; ++dy)
      for (int dx = -viewRadius_; dx <= viewRadius_; ++dx) {
        glm::ivec3 coord = pChunk + glm::ivec3{dx, dy, dz};
        auto &chunk = chunks_[coord];

        if (!chunk.volume) {
          // First time seeing this chunk: create voxel data + generate terrain
          chunk.volume =
              std::make_unique<engine::voxel::VoxelVolume>(CHUNK_DIM);
          TerrainGenerator::Generate(*chunk.volume, coord * CHUNK_DIM);
          chunk.dirty = true;
        }

        // Whether new or existing, check if we need to (re)mesh
        if (chunk.dirty && !chunk.meshJobQueued) {
          std::cout << "[ChunkManager] Queued mesh job for chunk at " << coord.x
                    << ", " << coord.y << ", " << coord.z << std::endl;

          auto *volPtr = chunk.volume.get();
          threadPool.enqueueMesh(
              coord, [volPtr]() { return VoxelMesher::GenerateMesh(*volPtr); });
          chunk.meshJobQueued = true;
        }
      }
}
