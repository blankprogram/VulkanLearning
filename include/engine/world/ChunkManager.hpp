
// include/engine/world/ChunkManager.hpp
#pragma once

#include "engine/utils/ThreadPool.hpp"
#include "engine/world/Chunk.hpp"
#include <glm/glm.hpp>
#include <unordered_map>
namespace engine::world {

class ChunkManager {
public:
  ChunkManager();
  // now takes your ThreadPool
  void initChunks(engine::utils::ThreadPool &threadPool);
  void updateChunks(const glm::vec3 &playerPos,
                    engine::utils::ThreadPool &threadPool);
  // for inserting finished meshes
  Chunk &getChunk(const glm::ivec3 &coord) { return chunks_[coord]; }

private:
  static constexpr glm::ivec3 CHUNK_DIM{16, 16, 16};
  int viewRadius_ = 8;
  std::unordered_map<glm::ivec3, Chunk> chunks_;
};

} // namespace engine::world
