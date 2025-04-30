#include "engine/world/TerrainGenerator.hpp"
#include "engine/voxel/VoxelVolume.hpp"
#include <externals/FastNoiseLite.h>
#include <glm/glm.hpp>
#include <random>

namespace engine::world {

void TerrainGenerator::Generate(engine::voxel::VoxelVolume &vol,
                                const glm::ivec3 &chunkCoord) {
  static FastNoiseLite baseNoise;
  static FastNoiseLite mountainNoise;
  baseNoise.SetSeed(1337);
  baseNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);

  mountainNoise.SetSeed(42);
  mountainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  mountainNoise.SetFrequency(0.03f);

  const glm::ivec3 ext = vol.extent;
  const int stoneLayer = 4;
  const int dirtLayer = 3;
  const int grassLayer = 1;

  std::mt19937 rng(chunkCoord.x * 73856093 ^ chunkCoord.z * 19349663);
  std::uniform_real_distribution<float> variation(-0.1f, 0.1f);

  for (int z = 0; z < ext.z; ++z) {
    for (int x = 0; x < ext.x; ++x) {
      float wx = float(chunkCoord.x + x);
      float wz = float(chunkCoord.z + z);

      float n = baseNoise.GetNoise(wx * 0.05f, wz * 0.05f);
      n = glm::clamp(n, -1.0f, 1.0f);
      int baseHeight =
          stoneLayer + dirtLayer + grassLayer + int(n * 6.0f); // 8–16

      // Add mountain on top
      float m = mountainNoise.GetNoise(wx, wz);
      m = glm::clamp(m, 0.0f, 1.0f);
      int mountainHeight = int(m * 32.0f); // up to 8 voxels above

      for (int y = 0; y < ext.y; ++y) {
        auto &voxel = vol.at(x, y, z);
        int worldY = chunkCoord.y + y;

        if (worldY <= baseHeight - (dirtLayer + grassLayer)) {
          // Stone layer (bottom)
          voxel.solid = true;
          float v = 0.4f + variation(rng);
          voxel.color = glm::vec3(v);
        } else if (worldY <= baseHeight - grassLayer) {
          // Dirt layer (middle)
          voxel.solid = true;
          float r = 0.4f + variation(rng);
          float g = 0.25f + variation(rng);
          float b = 0.1f + variation(rng);
          voxel.color = glm::vec3(r, g, b);
        } else if (worldY <= baseHeight) {
          // Grass layer (top)
          voxel.solid = true;
          float r = 0.2f + variation(rng);
          float g = 0.6f + variation(rng);
          float b = 0.2f + variation(rng);
          voxel.color = glm::vec3(r, g, b);
        } else if (worldY <= baseHeight + mountainHeight) {
          // Mountain (rocky/brownish)
          voxel.solid = true;
          float r = 0.3f + variation(rng);
          float g = 0.2f + variation(rng);
          float b = 0.1f + variation(rng);
          voxel.color = glm::vec3(r, g, b);
        } else {
          voxel.solid = false;
        }
      }
    }
  }
}

} // namespace engine::world
