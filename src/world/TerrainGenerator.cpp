
// engine/world/TerrainGenerator.cpp
#include "engine/world/TerrainGenerator.hpp"
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
  mountainNoise.SetFrequency(0.01f); // sparse, tall peaks

  const glm::ivec3 ext = vol.extent;
  const int stoneLayer = 4;
  const int dirtLayer = 3;
  const int grassLayer = 1;

  std::mt19937 rng(chunkCoord.x * 73856093 ^ chunkCoord.z * 19349663);
  std::uniform_real_distribution<float> variation(-0.1f, 0.1f);
  std::bernoulli_distribution treeDist(0.02f); // ~2% chance per grass tile
  const int minTrunkHeight = 4;
  const int maxTrunkHeight = 6;
  const int canopyRadius = 2;

  for (int z = 0; z < ext.z; ++z) {
    for (int x = 0; x < ext.x; ++x) {
      float wx = float(chunkCoord.x + x);
      float wz = float(chunkCoord.z + z);

      float n = baseNoise.GetNoise(wx * 0.05f, wz * 0.05f);
      n = glm::clamp(n, -1.0f, 1.0f);
      int baseHeight = stoneLayer + dirtLayer + grassLayer + int(n * 6.0f);

      float m = mountainNoise.GetNoise(wx, wz);
      m = glm::clamp(m, 0.0f, 1.0f);
      int mountainHeight = int(m * 32.0f);
      int mountainTopWorldY = baseHeight + mountainHeight;

      // 3) Fill column
      for (int y = 0; y < ext.y; ++y) {
        auto &voxel = vol.at(x, y, z);
        int worldY = chunkCoord.y + y;

        if (worldY <= baseHeight - (dirtLayer + grassLayer)) {
          voxel.solid = true;
          float v = 0.4f + variation(rng);
          voxel.color = glm::vec3(v); // stone
        } else if (worldY <= baseHeight - grassLayer) {
          voxel.solid = true;
          voxel.color = glm::vec3(0.4f + variation(rng), 0.25f + variation(rng),
                                  0.1f + variation(rng)); // dirt
        } else if (worldY <= baseHeight) {
          voxel.solid = true;
          voxel.color = glm::vec3(0.2f + variation(rng), 0.6f + variation(rng),
                                  0.2f + variation(rng)); // grass
        } else if (worldY <= mountainTopWorldY) {
          voxel.solid = true;
          int localH = worldY - baseHeight;
          if (mountainHeight >= 20 && localH >= (mountainHeight - 2)) {
            voxel.color = glm::vec3(0.95f); // snow cap
          } else {
            voxel.color =
                glm::vec3(0.3f + variation(rng), 0.2f + variation(rng),
                          0.1f + variation(rng)); // rock
          }
        } else {
          voxel.solid = false; // air
        }
      }

      if (mountainHeight == 0 && treeDist(rng)) {
        int trunkBaseY = (baseHeight - chunkCoord.y) + 1;
        int trunkH =
            minTrunkHeight + (rng() % (maxTrunkHeight - minTrunkHeight + 1));

        // trunk
        for (int i = 0; i < trunkH; ++i) {
          int yy = trunkBaseY + i;
          if (yy < 0 || yy >= ext.y)
            break;
          auto &logV = vol.at(x, yy, z);
          logV.solid = true;
          logV.color = glm::vec3(0.55f, 0.27f, 0.07f); // wood
        }

        int leafStartY = trunkBaseY + trunkH - 1;
        for (int ly = leafStartY; ly <= leafStartY + 2; ++ly) {
          if (ly < 0 || ly >= ext.y)
            continue;
          for (int lx = x - canopyRadius; lx <= x + canopyRadius; ++lx) {
            if (lx < 0 || lx >= ext.x)
              continue;
            for (int lz = z - canopyRadius; lz <= z + canopyRadius; ++lz) {
              if (lz < 0 || lz >= ext.z)
                continue;
              // prune bottom‐corners
              if (ly == leafStartY && abs(lx - x) == canopyRadius &&
                  abs(lz - z) == canopyRadius)
                continue;
              auto &leafV = vol.at(lx, ly, lz);
              leafV.solid = true;
              leafV.color = glm::vec3(0.0f, 0.8f, 0.0f); // leaves
            }
          }
        }
      }
    }
  }
}

} // namespace engine::world
