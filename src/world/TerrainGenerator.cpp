
// src/world/TerrainGenerator.cpp
#include "engine/world/TerrainGenerator.hpp"
#include "engine/voxel/VoxelVolume.hpp"

#include <algorithm>   // std::clamp
#include <cmath>       // std::sin, std::cos, std::floor
#include <glm/glm.hpp> // glm::ivec3

namespace engine::world {

void TerrainGenerator::Generate(engine::voxel::VoxelVolume &vol) {
  // extents of this chunk
  const glm::ivec3 ext = vol.extent;

  // parameters you can tweak
  const float frequency = 0.1f;          // controls wavelength
  const float amplitude = ext.y * 0.4f;  // max height variation
  const float baseHeight = ext.y * 0.5f; // vertical offset

  // for each (x,z) column, compute a height and fill voxels below it
  for (int z = 0; z < ext.z; ++z) {
    for (int x = 0; x < ext.x; ++x) {
      // simple height function
      float h = std::sin(x * frequency) + std::cos(z * frequency);
      h = h * amplitude + baseHeight;
      int height = std::clamp(static_cast<int>(std::floor(h)), 0, ext.y - 1);

      // fill all voxels up to 'height'
      for (int y = 0; y < ext.y; ++y) {
        vol.at(x, y, z).solid = (y <= height);
      }
    }
  }
}

} // namespace engine::world
