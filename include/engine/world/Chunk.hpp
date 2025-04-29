
#pragma once

#include "engine/render/Mesh.hpp"       // bring in Mesh
#include "engine/voxel/VoxelVolume.hpp" // bring in engine::voxel::VoxelVolume
#include <glm/vec3.hpp>
#include <memory>

namespace engine::world {

struct Chunk {
  glm::ivec3 chunkPos;
  std::unique_ptr<engine::voxel::VoxelVolume> volume; // voxel data
  std::unique_ptr<Mesh> mesh;                         // CPU mesh
  bool dirty = true;                                  // needs meshing
  bool meshJobQueued = false;                         // async flag
};

} // namespace engine::world
