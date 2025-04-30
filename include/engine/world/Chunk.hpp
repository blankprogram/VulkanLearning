
#pragma once

#include "engine/render/Mesh.hpp"
#include "engine/voxel/VoxelVolume.hpp"
#include <glm/vec2.hpp>
#include <memory>

namespace engine::world {

// CHUNK_DIM is still 3D, but chunk position is only 2D (XZ)
struct Chunk {
  glm::ivec2 coordXZ; // 2D chunk coordinate (X,Z)
  std::unique_ptr<engine::voxel::VoxelVolume> volume;
  std::unique_ptr<Mesh> mesh;
  bool dirty = true;
  bool meshJobQueued = false;
};

} // namespace engine::world
