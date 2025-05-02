#pragma once

#include "engine/voxel/VoxelVolume.hpp"

namespace engine::world {

class TerrainGenerator {
  public:
    static void Generate(engine::voxel::VoxelVolume &vol,
                         const glm::ivec3 &chunkCoord);
};

} // namespace engine::world
