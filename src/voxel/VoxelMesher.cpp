

// engine/voxel/VoxelMesher.cpp

#include "engine/voxel/VoxelMesher.hpp"
#include "engine/render/Vertex.hpp"
#include <glm/vec3.hpp>
#include <memory>
#include <vector>

using namespace engine;
using namespace engine::voxel;

std::unique_ptr<Mesh> VoxelMesher::GenerateMesh(const VoxelVolume &vol) {
  // 1) Dimensions of our volume
  const glm::ivec3 size = vol.extent;

  // 2) Buffers for output
  std::vector<Vertex> verts;
  std::vector<uint32_t> idxs;

  // 3) Iterate over the 3 principal axes: 0=X, 1=Y, 2=Z
  for (int d = 0; d < 3; ++d) {
    // u and v are the other two axes
    int u = (d + 1) % 3;
    int v = (d + 2) % 3;

    // extent along u and v
    const int U = size[u], V = size[v];
    std::vector<int> mask(U * V);

    // 4) Two passes: one for +faces (dir=1), one for −faces (dir=−1)
    for (int dir = 1; dir >= -1; dir -= 2) {
      // sweep through slices perpendicular to axis d
      for (int x = 0; x <= size[d]; ++x) {

        // 4a) Build the mask for this slice
        for (int j = 0; j < V; ++j) {
          for (int i = 0; i < U; ++i) {
            // voxel coords on either side of the plane
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

            // mask = ±1 if face needed, 0 otherwise
            if (va != vb) {
              // face normal should point from solid → empty
              mask[j * U + i] = (va ? dir : -dir);
            } else {
              mask[j * U + i] = 0;
            }
          }
        }

        // 4b) Greedy‐merge rectangles in the mask
        for (int j = 0; j < V; ++j) {
          for (int i = 0; i < U; ++i) {
            int m = mask[j * U + i];
            if (m == 0)
              continue;

            // 1) Compute width w
            int w = 1;
            while (i + w < U && mask[j * U + (i + w)] == m) {
              ++w;
            }

            // 2) Compute height h (at least 1!)
            int h = 1;
            bool rowOK = true;
            while (j + h < V && rowOK) {
              for (int k = 0; k < w; ++k) {
                if (mask[(j + h) * U + (i + k)] != m) {
                  rowOK = false;
                  break;
                }
              }
              if (rowOK)
                ++h;
            }

            // 3) Compute quad geometry
            glm::vec3 origin{0}, du{0}, dv{0}, normal{0};
            // position along the sweep axis
            origin[d] = float(x);
            // offsets in the u/v plane
            origin[u] = float(i);
            origin[v] = float(j);
            du[u] = float(w);
            dv[v] = float(h);
            normal[d] = float(m);

            // corners
            glm::vec3 p0 = origin;
            glm::vec3 p1 = origin + du;
            glm::vec3 p2 = origin + du + dv;
            glm::vec3 p3 = origin + dv;

            // 4) Emit vertices & indices
            uint32_t base = uint32_t(verts.size());
            if (m > 0) {
              // front‐facing winding
              verts.push_back({p0, normal, {0, 0}});
              verts.push_back({p1, normal, {float(w), 0}});
              verts.push_back({p2, normal, {float(w), float(h)}});
              verts.push_back({p3, normal, {0, float(h)}});
              idxs.insert(idxs.end(),
                          {base, base + 1, base + 2, base, base + 2, base + 3});
            } else {
              // back‐facing winding (flip p1/p3)
              verts.push_back({p0, normal, {0, 0}});
              verts.push_back({p3, normal, {0, float(h)}});
              verts.push_back({p2, normal, {float(w), float(h)}});
              verts.push_back({p1, normal, {float(w), 0}});
              idxs.insert(idxs.end(),
                          {base, base + 1, base + 2, base, base + 2, base + 3});
            }

            // 5) Zero‐out used mask so we don’t re‐emit
            for (int yy = 0; yy < h; ++yy) {
              for (int xx = 0; xx < w; ++xx) {
                mask[(j + yy) * U + (i + xx)] = 0;
              }
            }

            // advance i past this rectangle
            i += w - 1;
          }
        }
      }
    }
  }

  // 6) Pack into a Mesh and return
  auto mesh = std::make_unique<Mesh>();
  mesh->setVertices(std::move(verts));
  mesh->setIndices(std::move(idxs));
  return mesh;
}
