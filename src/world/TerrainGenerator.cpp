

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

  for (int z = 0; z < ext.z; ++z) {
    for (int x = 0; x < ext.x; ++x) {
      for (int y = 0; y < ext.y; ++y) {
        bool isGround = (y == 0); // You can use noise instead
        auto &voxel = vol.at(x, y, z);
        voxel.solid = isGround;

        if (isGround) {
          float hue = glm::fract((chunkCoord.x + x + chunkCoord.z + z) * 0.1f);
          voxel.color = glm::vec3(hue, 0.6f, 0.2f); // earthy tone
        }
      }
    }
  }
}
} // namespace engine::world
