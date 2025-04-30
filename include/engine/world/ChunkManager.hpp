
#pragma once

#include "engine/utils/ThreadPool.hpp"
#include "engine/world/Chunk.hpp"
#include <glm/glm.hpp>
#include <mutex>
#include <unordered_map>

namespace engine::world {

// A 2D integer vector hash for glm::ivec2
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

  // Access or create a Chunk by its coordinate
  Chunk &getChunk(const glm::ivec2 &coord) { return chunks_[coord]; }

  // Retrieves all loaded chunks
  const std::unordered_map<glm::ivec2, Chunk, ivec2_hash> &getChunks() const {
    return chunks_;
  }

  // Pending voxel volumes to assign on the main thread
  std::unordered_map<glm::ivec2, std::unique_ptr<engine::voxel::VoxelVolume>,
                     ivec2_hash>
      chunkVolumesPending_;

  mutable std::mutex assignMtx_; // Guards access to chunkVolumesPending_

private:
  static constexpr glm::ivec3 CHUNK_DIM{16, 256, 16};
  int viewRadius_ = 3;

  std::unordered_map<glm::ivec2, Chunk, ivec2_hash> chunks_;
};

} // namespace engine::world
