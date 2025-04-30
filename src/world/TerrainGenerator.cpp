
#include "engine/world/TerrainGenerator.hpp"
#include "engine/voxel/VoxelVolume.hpp"
#include <externals/FastNoiseLite.h>
#include <glm/glm.hpp>

namespace engine::world {

void TerrainGenerator::Generate(engine::voxel::VoxelVolume &vol,
                                const glm::ivec3 &chunkCoord) {
  static FastNoiseLite colorNoise;
  colorNoise.SetSeed(1337);
  colorNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

  glm::ivec3 ext = vol.extent;
  glm::ivec3 base = chunkCoord * ext;

  for (int z = 0; z < ext.z; ++z) {
    for (int x = 0; x < ext.x; ++x) {
      for (int y = 0; y < ext.y; ++y) {
        auto &voxel = vol.at(x, y, z);
        int worldY = base.y + y;

        if (worldY == 0) {
          voxel.solid = true;
          glm::vec3 worldPos = glm::vec3(base + glm::ivec3{x, y, z});
          float r = 0.5f + 0.5f * colorNoise.GetNoise(worldPos.x * 0.3f,
                                                      worldPos.y * 0.3f,
                                                      worldPos.z * 0.3f);
          float g = 0.5f + 0.5f * colorNoise.GetNoise(
                                      worldPos.x * 0.3f + 100.0f,
                                      worldPos.y * 0.3f, worldPos.z * 0.3f);
          float b = 0.5f + 0.5f * colorNoise.GetNoise(
                                      worldPos.x * 0.3f, worldPos.y * 0.3f,
                                      worldPos.z * 0.3f + 100.0f);
          voxel.color = glm::vec3(r, g, b);
        } else {
          voxel.solid = false;
        }
      }
    }
  }
}

} // namespace engine::world
