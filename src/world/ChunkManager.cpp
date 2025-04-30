

// src/world/ChunkManager.cpp
#include "engine/world/ChunkManager.hpp"
#include "engine/voxel/VoxelMesher.hpp"
#include "engine/world/TerrainGenerator.hpp"
#include <glm/glm.hpp>
#include <iostream>

using namespace engine::world;
using engine::voxel::VoxelMesher;

ChunkManager::ChunkManager() = default;

void ChunkManager::initChunks(engine::utils::ThreadPool &threadPool) {
  updateChunks(glm::vec3{0, 0, 0}, threadPool);
}

void ChunkManager::updateChunks(const glm::vec3 &playerPos,
                                engine::utils::ThreadPool &threadPool) {
  // Fix: Correct chunk coord calculation from world position
  glm::ivec2 playerChunkXZ =
      glm::ivec2(glm::floor(playerPos.x / float(CHUNK_DIM.x)),
                 glm::floor(playerPos.z / float(CHUNK_DIM.z)));

  for (int dz = -viewRadius_; dz <= viewRadius_; ++dz) {
    for (int dx = -viewRadius_; dx <= viewRadius_; ++dx) {
      glm::ivec2 coordXZ = playerChunkXZ + glm::ivec2(dx, dz);
      auto &chunk = chunks_[coordXZ];

      if (!chunk.volume) {
        chunk.volume = std::make_unique<engine::voxel::VoxelVolume>(CHUNK_DIM);
        glm::ivec3 chunkOrigin =
            glm::ivec3(coordXZ.x * CHUNK_DIM.x, 0, coordXZ.y * CHUNK_DIM.z);
        TerrainGenerator::Generate(*chunk.volume, chunkOrigin);
        chunk.dirty = true;
      }

      if (chunk.dirty && !chunk.meshJobQueued && !chunk.mesh) {
        auto copy = std::make_shared<engine::voxel::VoxelVolume>(*chunk.volume);
        threadPool.enqueueMesh(glm::ivec3(coordXZ.x, 0, coordXZ.y), [copy]() {
          return VoxelMesher::GenerateMesh(*copy);
        });
        chunk.meshJobQueued = true;
      }
    }
  }
}
