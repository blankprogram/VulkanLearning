// src/world/ChunkManager.cpp
#include "engine/world/ChunkManager.hpp"
#include "engine/voxel/VoxelMesher.hpp"
#include "engine/voxel/VoxelVolume.hpp"
#include "engine/world/TerrainGenerator.hpp"

#include <cmath> // std::floor
#include <glm/glm.hpp>

namespace engine::world {

void ChunkManager::initChunks() { updateChunks(glm::vec3{0.0f, 0.0f, 0.0f}); }

void ChunkManager::updateChunks(const glm::vec3 &playerPos) {
  // convert to integral chunk coords
  glm::ivec3 pChunk{static_cast<int>(std::floor(playerPos.x)),
                    static_cast<int>(std::floor(playerPos.y)),
                    static_cast<int>(std::floor(playerPos.z))};

  for (int dz = -viewRadius_; dz <= viewRadius_; ++dz) {
    for (int dy = -viewRadius_; dy <= viewRadius_; ++dy) {
      for (int dx = -viewRadius_; dx <= viewRadius_; ++dx) {
        glm::ivec3 coord = pChunk + glm::ivec3{dx, dy, dz};
        auto &chunk = chunks_[coord];

        // allocate and generate voxel data if missing
        if (!chunk.volume) {
          chunk.volume =
              std::make_unique<engine::voxel::VoxelVolume>(CHUNK_DIM);
          TerrainGenerator::Generate(*chunk.volume);
          chunk.dirty = true;
        }

        // mesh if needed
        if (chunk.dirty) {
          chunk.mesh = engine::voxel::VoxelMesher::GenerateMesh(*chunk.volume);
          chunk.dirty = false;
        }
      }
    }
  }
}

} // namespace engine::world
