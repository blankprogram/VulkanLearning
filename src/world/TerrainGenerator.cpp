

// src/world/TerrainGenerator.cpp
#include "engine/world/TerrainGenerator.hpp"
#include "engine/voxel/VoxelVolume.hpp"

#include <algorithm>             // std::clamp
#include <cmath>                 // std::sin, std::cos, std::floor
#include <glm/glm.hpp>           // glm::ivec3
#include <glm/gtc/constants.hpp> // glm::pi, glm::two_pi

namespace engine::world {

void TerrainGenerator::Generate(engine::voxel::VoxelVolume &vol,
                                const glm::ivec3 &chunkCoord) {
  const glm::ivec3 ext = vol.extent;

  // A single horizontal slice at y = 0
  for (int z = 0; z < ext.z; ++z) {
    for (int x = 0; x < ext.x; ++x) {
      for (int y = 0; y < ext.y; ++y) {
        vol.at(x, y, z).solid = (y == 0);
      }
    }
  }
}

} // namespace engine::world
