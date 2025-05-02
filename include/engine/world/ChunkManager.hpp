
#pragma once

#include "engine/utils/ThreadPool.hpp"
#include "engine/world/Chunk.hpp"
#include <glm/glm.hpp>
#include <mutex>
#include <unordered_map>

namespace engine::world {

struct ivec2_hash {
  std::size_t operator()(const glm::ivec2 &v) const noexcept {
    return (std::hash<int>()(v.x) * 73856093u) ^
           (std::hash<int>()(v.y) * 19349663u);
  }
};

class ChunkManager {
public:
  ChunkManager();
  void initChunks(engine::utils::ThreadPool &threadPool);
  void updateChunks(const glm::vec3 &playerPos,
                    engine::utils::ThreadPool &threadPool);

  Chunk &getChunk(const glm::ivec2 &coord) { return chunks_[coord]; }

  const std::unordered_map<glm::ivec2, Chunk, ivec2_hash> &getChunks() const {
    return chunks_;
  }

  std::unordered_map<glm::ivec2, std::unique_ptr<engine::voxel::VoxelVolume>,
                     ivec2_hash>
      chunkVolumesPending_;

  mutable std::mutex assignMtx_;

private:
  std::unordered_map<glm::ivec2, Chunk, ivec2_hash> chunks_;
};

} // namespace engine::world
