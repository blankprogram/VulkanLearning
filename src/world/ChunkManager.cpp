

#include "engine/world/ChunkManager.hpp"
#include "engine/voxel/VoxelMesher.hpp"
#include "engine/world/Config.hpp"
#include "engine/world/TerrainGenerator.hpp"

using namespace engine::world;

ChunkManager::ChunkManager() = default;

void ChunkManager::initChunks(engine::utils::ThreadPool &threadPool) {
  updateChunks(glm::vec3{0, 0, 0}, threadPool);
}

void ChunkManager::updateChunks(const glm::vec3 &playerPos,
                                engine::utils::ThreadPool &threadPool) {
  glm::ivec2 playerChunk =
      glm::ivec2(glm::floor(playerPos.x / float(CHUNK_DIM.x)),
                 glm::floor(playerPos.z / float(CHUNK_DIM.z)));

  for (int dz = -VIEW_RADIUS; dz <= VIEW_RADIUS; ++dz) {
    for (int dx = -VIEW_RADIUS; dx <= VIEW_RADIUS; ++dx) {
      glm::ivec2 coord = playerChunk + glm::ivec2(dx, dz);
      Chunk &chunk = chunks_[coord];

      if (!chunk.volume && !chunk.meshJobQueued) {
        chunk.meshJobQueued = true;
        glm::ivec3 chunkOrigin(coord.x * CHUNK_DIM.x, 0, coord.y * CHUNK_DIM.z);
        auto coordCopy = coord;

        threadPool.enqueueJob([this, coordCopy, chunkOrigin, &threadPool]() {
          auto volume = std::make_unique<engine::voxel::VoxelVolume>(CHUNK_DIM);
          engine::world::TerrainGenerator::Generate(*volume, chunkOrigin);

          auto meshJob =
              [volumeCopy =
                   std::make_shared<engine::voxel::VoxelVolume>(*volume)]() {
                return engine::voxel::VoxelMesher::GenerateMesh(*volumeCopy);
              };

          threadPool.enqueueMesh(glm::ivec3(coordCopy.x, 0, coordCopy.y),
                                 meshJob);

          std::lock_guard<std::mutex> lock(assignMtx_);
          chunkVolumesPending_.emplace(coordCopy, std::move(volume));
        });
      }
    }
  }
}
