#pragma once

#include "engine/voxel/VoxelVolume.hpp"

namespace engine::world {

class TerrainGenerator {
public:
  // Procedurally fill the volume (e.g., heightmap, noise)
  static void Generate(engine::voxel::VoxelVolume &volume);
};

} // namespace engine::world
