

// engine/voxel/VoxelMesher.cpp

#include "engine/voxel/VoxelMesher.hpp"
#include "engine/render/Vertex.hpp"
#include <glm/vec3.hpp>
#include <memory>
#include <vector>

using namespace engine;
using namespace engine::voxel;

std::unique_ptr<Mesh> VoxelMesher::GenerateMesh(const VoxelVolume &vol) {
  const glm::ivec3 size = vol.extent;
  std::vector<Vertex> verts;
  std::vector<uint32_t> idxs;

  for (int d = 0; d < 3; ++d) {
    int u = (d + 1) % 3;
    int v = (d + 2) % 3;

    const int U = size[u], V = size[v];
    std::vector<int> mask(U * V);
    std::vector<glm::vec3> faceColorMask(U * V);

    for (int dir = 1; dir >= -1; dir -= 2) {
      for (int x = 0; x <= size[d]; ++x) {

        // --- Build mask and color mask ---
        for (int j = 0; j < V; ++j) {
          for (int i = 0; i < U; ++i) {
            glm::ivec3 a{0}, b{0};
            a[d] = x - (dir == 1);
            b[d] = x;
            a[u] = b[u] = i;
            a[v] = b[v] = j;

            auto inBounds = [&](const glm::ivec3 &p) {
              return p.x >= 0 && p.y >= 0 && p.z >= 0 && p.x < size.x &&
                     p.y < size.y && p.z < size.z;
            };

            bool va = inBounds(a) ? vol.at(a.x, a.y, a.z).solid : false;
            bool vb = inBounds(b) ? vol.at(b.x, b.y, b.z).solid : false;

            if (va != vb) {
              mask[j * U + i] = (va ? dir : -dir);
              glm::ivec3 voxelCoord = va ? a : b;
              faceColorMask[j * U + i] =
                  vol.at(voxelCoord.x, voxelCoord.y, voxelCoord.z).color;
            } else {
              mask[j * U + i] = 0;
              faceColorMask[j * U + i] = glm::vec3(0.0f);
            }
          }
        }

        // --- Greedy merging ---
        for (int j = 0; j < V; ++j) {
          for (int i = 0; i < U; ++i) {
            int m = mask[j * U + i];
            if (m == 0)
              continue;

            glm::vec3 currentColor = faceColorMask[j * U + i];

            // 1) Compute width
            int w = 1;
            while (i + w < U && mask[j * U + (i + w)] == m &&
                   faceColorMask[j * U + (i + w)] == currentColor) {
              ++w;
            }

            // 2) Compute height
            int h = 1;
            bool rowOK = true;
            while (j + h < V && rowOK) {
              for (int k = 0; k < w; ++k) {
                int idx = (j + h) * U + (i + k);
                if (mask[idx] != m || faceColorMask[idx] != currentColor) {
                  rowOK = false;
                  break;
                }
              }
              if (rowOK)
                ++h;
            }

            // 3) Quad geometry
            glm::vec3 origin{0}, du{0}, dv{0}, normal{0};
            origin[d] = float(x);
            origin[u] = float(i);
            origin[v] = float(j);
            du[u] = float(w);
            dv[v] = float(h);
            normal[d] = float(m);

            glm::vec3 p0 = origin;
            glm::vec3 p1 = origin + du;
            glm::vec3 p2 = origin + du + dv;
            glm::vec3 p3 = origin + dv;

            uint32_t base = uint32_t(verts.size());

            if (m > 0) {
              verts.push_back({p0, normal, {0, 0}, currentColor});
              verts.push_back({p1, normal, {float(w), 0}, currentColor});
              verts.push_back({p2, normal, {float(w), float(h)}, currentColor});
              verts.push_back({p3, normal, {0, float(h)}, currentColor});
              idxs.insert(idxs.end(),
                          {base, base + 1, base + 2, base, base + 2, base + 3});
            } else {
              verts.push_back({p0, normal, {0, 0}, currentColor});
              verts.push_back({p3, normal, {0, float(h)}, currentColor});
              verts.push_back({p2, normal, {float(w), float(h)}, currentColor});
              verts.push_back({p1, normal, {float(w), 0}, currentColor});
              idxs.insert(idxs.end(),
                          {base, base + 1, base + 2, base, base + 2, base + 3});
            }

            // 4) Clear mask
            for (int yy = 0; yy < h; ++yy) {
              for (int xx = 0; xx < w; ++xx) {
                int idx = (j + yy) * U + (i + xx);
                mask[idx] = 0;
                faceColorMask[idx] = glm::vec3(0.0f);
              }
            }

            i += w - 1;
          }
        }
      }
    }
  }

  auto mesh = std::make_unique<Mesh>();
  mesh->setVertices(std::move(verts));
  mesh->setIndices(std::move(idxs));
  return mesh;
}
