
// src/world/TerrainGenerator.cpp
#include "engine/world/TerrainGenerator.hpp"
#include "engine/voxel/VoxelVolume.hpp"
#include <externals/FastNoiseLite.h>
#include <glm/glm.hpp>

namespace engine::world {

void TerrainGenerator::Generate(engine::voxel::VoxelVolume &vol,
                                const glm::ivec3 &chunkCoord) {
  static FastNoiseLite noise;
  noise.SetSeed(1337);
  noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

  const glm::ivec3 ext = vol.extent;
  const int baseY = chunkCoord.y;

  for (int z = 0; z < ext.z; ++z) {
    for (int x = 0; x < ext.x; ++x) {
      glm::vec3 worldXZ = glm::vec3(chunkCoord.x + x, 0.0f, chunkCoord.z + z);
      float n = noise.GetNoise(worldXZ.x * 0.05f, worldXZ.z * 0.05f);
      n = glm::clamp(n, -1.0f, 1.0f);

      int height = 4 + int(n * 8.0f); // between ~[-4,+4] → [0,8]

      for (int y = 0; y < ext.y; ++y) {
        auto &voxel = vol.at(x, y, z);
        if (y <= height) {
          voxel.solid = true;
          float shade = 0.4f + 0.6f * (float(y) / ext.y);
          voxel.color = glm::vec3(shade);
        } else {
          voxel.solid = false;
        }
      }
    }
  }
}

} // namespace engine::world
