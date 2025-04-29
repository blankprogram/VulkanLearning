#include "engine/voxel/VoxelMesher.hpp"
#include "engine/render/Vertex.hpp"
#include <glm/vec3.hpp>

using namespace engine;
using namespace engine::voxel;

std::unique_ptr<Mesh> VoxelMesher::GenerateMesh(const VoxelVolume &vol) {
  std::vector<Vertex> verts;
  std::vector<uint32_t> idxs;

  auto inBounds = [&](int x, int y, int z) {
    return x >= 0 && y >= 0 && z >= 0 && x < vol.extent.x && y < vol.extent.y &&
           z < vol.extent.z;
  };
  auto isSolid = [&](int x, int y, int z) {
    return inBounds(x, y, z) ? vol.at(x, y, z).solid : false;
  };

  // for each voxel
  for (int z = 0; z < vol.extent.z; ++z)
    for (int y = 0; y < vol.extent.y; ++y)
      for (int x = 0; x < vol.extent.x; ++x) {
        if (!isSolid(x, y, z))
          continue;
        glm::vec3 p(x, y, z);
        // check 6 faces
        struct Face {
          glm::ivec3 offset;
          glm::vec3 dir;
          glm::vec3 u, v;
        };
        static Face faces[6] = {
            {{0, 0, -1}, {0, 0, -1}, {1, 0, 0}, {0, 1, 0}}, // back
            {{0, 0, 1}, {0, 0, 1}, {1, 0, 0}, {0, 1, 0}},   // front
            {{-1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 1, 0}}, // left
            {{1, 0, 0}, {1, 0, 0}, {0, 0, 1}, {0, 1, 0}},   // right
            {{0, -1, 0}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}}, // bottom
            {{0, 1, 0}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}}    // top
        };
        for (int f = 0; f < 6; ++f) {
          auto &face = faces[f];
          int nx = x + face.offset.x;
          int ny = y + face.offset.y;
          int nz = z + face.offset.z;
          if (isSolid(nx, ny, nz))
            continue;
          // add face quad
          uint32_t base = verts.size();
          glm::vec3 normal = face.dir;
          glm::vec3 corners[4] = {p, p + face.u, p + face.u + face.v,
                                  p + face.v};
          glm::vec2 uvs[4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
          for (int i = 0; i < 4; ++i) {
            verts.push_back({corners[i], normal, uvs[i]});
          }
          // two triangles
          idxs.insert(idxs.end(),
                      {base, base + 1, base + 2, base, base + 2, base + 3});
        }
      }

  auto mesh = std::make_unique<Mesh>();
  mesh->setVertices(std::move(verts));
  mesh->setIndices(std::move(idxs));
  return mesh;
}
