
#pragma once

#include "engine/render/Mesh.hpp"
#include "engine/voxel/VoxelVolume.hpp"

struct Chunk {
  glm::ivec3 chunkPos; // coordinate in chunk-space
  std::unique_ptr<VoxelVolume> volume;
  std::unique_ptr<Mesh> mesh;
  bool dirty = true; // true if needs remeshing
};
