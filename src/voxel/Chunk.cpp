
#include "engine/voxel/Chunk.hpp"
#include <glm/glm.hpp>

Chunk::Chunk(VkDevice device, VkPhysicalDevice physicalDevice,
             VkCommandPool commandPool, VkQueue graphicsQueue)
    : device_(device), physicalDevice_(physicalDevice),
      commandPool_(commandPool), graphicsQueue_(graphicsQueue) {
  // Only fill the bottom layer (y == 0) with solid blocks
  for (int x = 0; x < SIZE; ++x)
    for (int y = 0; y < SIZE; ++y)
      for (int z = 0; z < SIZE; ++z)
        blocks_[x][y][z].solid = (y == 0); // flat 16x16 layer
}

void Chunk::generateMesh() {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  uint32_t indexOffset = 0;

  // Face directions for neighbor checks
  constexpr glm::ivec3 directions[6] = {
      {0, 0, 1},  // front
      {0, 0, -1}, // back
      {-1, 0, 0}, // left
      {1, 0, 0},  // right
      {0, 1, 0},  // top
      {0, -1, 0}, // bottom
  };

  // Cube vertices (8 corners)
  constexpr glm::vec3 cubeVerts[8] = {
      {0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
      {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1},
  };

  // Indices for each face (two triangles)
  constexpr uint32_t faceIndices[6][6] = {
      {4, 5, 6, 6, 7, 4}, // front
      {1, 0, 3, 3, 2, 1}, // back
      {0, 4, 7, 7, 3, 0}, // left
      {5, 1, 2, 2, 6, 5}, // right
      {3, 7, 6, 6, 2, 3}, // top
      {0, 1, 5, 5, 4, 0}, // bottom
  };

  // Basic quad UVs
  static constexpr glm::vec2 uvs[4] = {
      {0.0f, 0.0f}, // bottom-left
      {1.0f, 0.0f}, // bottom-right
      {1.0f, 1.0f}, // top-right
      {0.0f, 1.0f}, // top-left
  };

  static constexpr int uvOrder[6] = {0, 1, 2, 2, 3, 0};

  for (int x = 0; x < SIZE; ++x) {
    for (int y = 0; y < SIZE; ++y) {
      for (int z = 0; z < SIZE; ++z) {
        if (!blocks_[x][y][z].solid)
          continue;

        glm::vec3 blockPos = {float(x), float(y), float(z)};

        for (int dir = 0; dir < 6; ++dir) {
          int nx = x + directions[dir].x;
          int ny = y + directions[dir].y;
          int nz = z + directions[dir].z;

          bool neighborSolid = (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE &&
                                nz >= 0 && nz < SIZE)
                                   ? blocks_[nx][ny][nz].solid
                                   : false;

          if (!neighborSolid) {
            // Exposed face → add vertices with correct UVs
            for (int i = 0; i < 6; ++i) {
              glm::vec3 pos = cubeVerts[faceIndices[dir][i]] + blockPos;
              glm::vec2 uv = uvs[uvOrder[i]];
              vertices.push_back(Vertex{{pos.x, pos.y, pos.z}, {uv.x, uv.y}});
              indices.push_back(indexOffset++);
            }
          }
        }
      }
    }
  }

  if (!vertices.empty()) {
    mesh_ = std::make_unique<Mesh>(device_, physicalDevice_, commandPool_,
                                   graphicsQueue_, vertices, indices);
  }
}
