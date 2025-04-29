#pragma once

#include "engine/world/Chunk.hpp"
#include <glm/glm.hpp> // glm::vec3, glm::ivec3
#include <unordered_map>

namespace engine::world {

class ChunkManager {
public:
  ChunkManager();

  // Generate & mesh chunks around player
  void updateChunks(const glm::vec3 &playerPos);
  void initChunks();

private:
  static constexpr glm::ivec3 CHUNK_DIM{16, 16, 16};
  int viewRadius_ = 8;
  std::unordered_map<glm::ivec3, Chunk> chunks_;
};

} // namespace engine::world
