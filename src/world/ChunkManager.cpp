#include "engine/world/ChunkManager.hpp"
#include "engine/voxel/VoxelMesher.hpp"
#include "engine/world/TerrainGenerator.hpp"

#include <cmath> // std::floor
#include <glm/glm.hpp>

using namespace engine::world;
using engine::voxel::VoxelMesher;

ChunkManager::ChunkManager() = default;
void ChunkManager::initChunks(engine::utils::ThreadPool &threadPool) {
  // generate initial chunks around origin
  updateChunks(glm::vec3{3, 3, 3}, threadPool);
}
void ChunkManager::updateChunks(const glm::vec3 &playerPos,
                                engine::utils::ThreadPool &threadPool) {
  // compute player chunk
  glm::ivec3 pChunk{int(std::floor(playerPos.x)), int(std::floor(playerPos.y)),
                    int(std::floor(playerPos.z))};

  for (int dz = -viewRadius_; dz <= viewRadius_; ++dz)
    for (int dy = -viewRadius_; dy <= viewRadius_; ++dy)
      for (int dx = -viewRadius_; dx <= viewRadius_; ++dx) {
        glm::ivec3 coord = pChunk + glm::ivec3{dx, dy, dz};
        auto &chunk = chunks_[coord];

        // allocate & generate volume
        if (!chunk.volume) {
          chunk.volume =
              std::make_unique<engine::voxel::VoxelVolume>(CHUNK_DIM);
          TerrainGenerator::Generate(*chunk.volume, coord);
          chunk.dirty = true;
          chunk.meshJobQueued = false;
        }

        // enqueue mesh job if needed
        if (chunk.dirty && !chunk.meshJobQueued) {
          // capture raw pointer to volume
          auto *volPtr = chunk.volume.get();
          threadPool.enqueueMesh(
              coord, [volPtr]() { return VoxelMesher::GenerateMesh(*volPtr); });
          chunk.meshJobQueued = true;
        }
      }
}
