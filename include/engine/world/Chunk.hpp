
// include/engine/world/Chunk.hpp
#pragma once

#include "engine/render/Mesh.hpp"       // bring in Mesh
#include "engine/voxel/VoxelVolume.hpp" // bring in engine::voxel::VoxelVolume
#include <glm/vec3.hpp>
#include <memory>

namespace engine::world {

struct Chunk {
  glm::ivec3 chunkPos;
  std::unique_ptr<engine::voxel::VoxelVolume> volume; // now known
  std::unique_ptr<Mesh> mesh; // Mesh is in global namespace
  bool dirty = true;
};

} // namespace engine::world
