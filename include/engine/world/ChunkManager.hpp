
// include/engine/world/ChunkManager.hpp
#pragma once

#include "engine/world/Chunk.hpp"
#include <glm/glm.hpp>
#include <unordered_map>

namespace engine::world {

class ChunkManager {
public:
  ChunkManager();

  void initChunks();
  void updateChunks(const glm::vec3 &playerPos,
                    engine::utils::ThreadPool &threadPool);

  // ← Add this:
  const auto &getChunks() const { return chunks_; }

private:
  static constexpr glm::ivec3 CHUNK_DIM{16, 16, 16};
  int viewRadius_ = 8;
  std::unordered_map<glm::ivec3, Chunk> chunks_;
};

} // namespace engine::world
