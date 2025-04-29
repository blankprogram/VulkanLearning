
#pragma once

#include <glm/vec3.hpp>
#include <memory>

class VoxelVolume;

struct Chunk {
  glm::ivec3 chunkPos;
  std::unique_ptr<VoxelVolume> volume; // assume managed internally
  std::unique_ptr<class Mesh> mesh;    // optional, if mesh is generated
  bool dirty = true;                   // example flag
};
