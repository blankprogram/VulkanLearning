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

  std::cout << "Total chunks tracked: " << chunks_.size() << "\n";
  glm::ivec3 pChunk = glm::ivec3(glm::floor(playerPos.x / float(CHUNK_DIM.x)),
                                 glm::floor(playerPos.y / float(CHUNK_DIM.y)),
                                 glm::floor(playerPos.z / float(CHUNK_DIM.z)));

  for (int dz = -viewRadius_; dz <= viewRadius_; ++dz)
    for (int dy = -viewRadius_; dy <= viewRadius_; ++dy)
      for (int dx = -viewRadius_; dx <= viewRadius_; ++dx) {

        glm::ivec3 coord = pChunk + glm::ivec3{dx, dy, dz};
        auto &chunk = chunks_[coord];

        if (!chunk.volume) {
          chunk.volume =
              std::make_unique<engine::voxel::VoxelVolume>(CHUNK_DIM);
          TerrainGenerator::Generate(*chunk.volume, coord * CHUNK_DIM);
          chunk.dirty = true;
          std::cout << "[ChunkManager] Generated terrain for " << coord.x
                    << ", " << coord.y << ", " << coord.z << "\n";
        }

        // ✅ Guard: Don't queue mesh job if mesh already exists
        if (chunk.dirty && !chunk.meshJobQueued && !chunk.mesh) {
          std::cout << "[ChunkManager] Queued mesh job for " << coord.x << ", "
                    << coord.y << ", " << coord.z << std::endl;

          auto volumeCopy =
              std::make_shared<engine::voxel::VoxelVolume>(*chunk.volume);
          threadPool.enqueueMesh(coord, [volumeCopy]() {
            return VoxelMesher::GenerateMesh(*volumeCopy);
          });

          chunk.meshJobQueued = true;
        }
      }
}
