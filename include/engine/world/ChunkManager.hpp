
// include/engine/world/ChunkManager.hpp
#pragma once

#include "engine/utils/ThreadPool.hpp"
#include "engine/world/Chunk.hpp"
#include <glm/glm.hpp>
#include <unordered_map>

namespace engine::world {

// A trivial but OK 3D-int vector hash:
struct ivec3_hash {
  std::size_t operator()(glm::ivec3 const &v) const noexcept {
    // pick some large primes
    return (std::hash<int>()(v.x) * 73856093u) ^
           (std::hash<int>()(v.y) * 19349663u) ^
           (std::hash<int>()(v.z) * 83492791u);
  }
};

class ChunkManager {
public:
  ChunkManager();
  void initChunks(engine::utils::ThreadPool &threadPool);
  void updateChunks(const glm::vec3 &playerPos,
                    engine::utils::ThreadPool &threadPool);
  Chunk &getChunk(const glm::ivec3 &coord) { return chunks_[coord]; }

  const std::unordered_map<glm::ivec3, Chunk, ivec3_hash> &getChunks() const {
    return chunks_;
  }

private:
  static constexpr glm::ivec3 CHUNK_DIM{16, 16, 16};
  int viewRadius_ = 4;

  // <-- note the third template parameter
  std::unordered_map<glm::ivec3, Chunk, ivec3_hash> chunks_;
};

} // namespace engine::world
