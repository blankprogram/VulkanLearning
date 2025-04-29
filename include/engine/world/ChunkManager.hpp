#include "engine/voxel/Voxel.hpp"
#include "engine/world/Chunk.hpp"
#include <glm/glm.hpp>
#include <unordered_map>
class ChunkManager {
public:
  ChunkManager();
  void initChunks();
  void updateChunks(const glm::vec3 &playerPos);

private:
  std::unordered_map<glm::ivec3, Chunk> chunks_;
};
