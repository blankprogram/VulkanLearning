
#include "engine/world/TerrainGenerator.hpp"
#include "engine/voxel/VoxelVolume.hpp"
#include <externals/FastNoiseLite.h>
#include <glm/glm.hpp>

namespace engine::world {

void TerrainGenerator::Generate(engine::voxel::VoxelVolume &vol,
                                const glm::ivec3 &chunkCoord) {
  static FastNoiseLite noise;
  noise.SetSeed(1337); // fixed seed for consistent colors
  noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

  glm::ivec3 ext = vol.extent;

  for (int z = 0; z < ext.z; ++z) {
    for (int x = 0; x < ext.x; ++x) {
      for (int y = 0; y < ext.y; ++y) {
        auto &voxel = vol.at(x, y, z);

        // Only fill y=0 layer with voxels
        if (y == 0) {
          voxel.solid = true;

          glm::ivec3 worldPos = chunkCoord + glm::ivec3{x, y, z};
          float noiseVal = noise.GetNoise((float)worldPos.x * 0.1f,
                                          (float)worldPos.z * 0.1f);
          float grey = glm::clamp(0.3f + 0.7f * noiseVal, 0.0f, 1.0f);
          voxel.color = glm::vec3(grey);
        } else {
          voxel.solid = false;
        }
      }
    }
  }
}

} // namespace engine::world
