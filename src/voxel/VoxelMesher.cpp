
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

  // For each dimension d = 0 (X),1 (Y),2 (Z)
  for (int d = 0; d < 3; ++d) {
    int u = (d + 1) % 3;
    int v = (d + 2) % 3;
    glm::ivec3 dims = size;
    // we’ll sweep from slice 0..size[d], so size[d]+1 planes
    std::vector<int> mask((dims[u]) * (dims[v]));

    // both directions: +1 face (mask=+1) and -1 face (mask=-1)
    for (int dir = 1; dir >= -1; dir -= 2) {
      for (int x = 0; x <= size[d]; ++x) {
        // build mask for this slice
        for (int j = 0; j < dims[v]; ++j) {
          for (int i = 0; i < dims[u]; ++i) {
            // voxel coords a and b on either side of plane
            glm::ivec3 a{0}, b{0};
            a[d] = x - (dir == 1);
            b[d] = x - (dir == -1);
            a[u] = b[u] = i;
            a[v] = b[v] = j;

            bool va = (a.x >= 0 && a.y >= 0 && a.z >= 0 && a.x < size.x &&
                       a.y < size.y && a.z < size.z)
                          ? vol.at(a.x, a.y, a.z).solid
                          : false;
            bool vb = (b.x >= 0 && b.y >= 0 && b.z >= 0 && b.x < size.x &&
                       b.y < size.y && b.z < size.z)
                          ? vol.at(b.x, b.y, b.z).solid
                          : false;
            // we want faces where va!=vb, and face normal points from
            // filled->empty
            mask[j * dims[u] + i] = (va != vb) ? (va ? dir : -dir) : 0;
          }
        }

        // now greedy‐merge rectangles in mask
        for (int j = 0; j < dims[v]; ++j) {
          for (int i = 0; i < dims[u]; ++i) {
            int m = mask[j * dims[u] + i];
            if (m == 0)
              continue;
            // find width
            int w;
            for (w = 1; i + w < dims[u] && mask[j * dims[u] + i + w] == m; ++w)
              ;
            // find height
            int h;
            bool done = false;
            for (h = 1; j + h < dims[v] && !done; ++h) {
              for (int k = 0; k < w; ++k) {
                if (mask[(j + h) * dims[u] + i + k] != m) {
                  done = true;
                  break;
                }
              }
            }
            --h;

            // compute quad corners
            glm::vec3 du{0}, dv{0}, origin{0};
            du[u] = (float)w;
            dv[v] = (float)h;
            origin[d] = (float)x;
            origin[u] = (float)i;
            origin[v] = (float)j;

            // normal
            glm::vec3 normal{0};
            normal[d] = (float)m;

            // four corners
            glm::vec3 p0 = origin;
            glm::vec3 p1 = origin + du;
            glm::vec3 p2 = origin + du + dv;
            glm::vec3 p3 = origin + dv;

            // push vertices and indices
            uint32_t base = (uint32_t)verts.size();
            if (m > 0) {
              verts.push_back({p0, normal, {0, 0}});
              verts.push_back({p1, normal, {w, 0}});
              verts.push_back({p2, normal, {w, h}});
              verts.push_back({p3, normal, {0, h}});
              idxs.insert(idxs.end(),
                          {base, base + 1, base + 2, base, base + 2, base + 3});
            } else {
              // flip for negative faces
              verts.push_back({p0, normal, {0, 0}});
              verts.push_back({p3, normal, {0, h}});
              verts.push_back({p2, normal, {w, h}});
              verts.push_back({p1, normal, {w, 0}});
              idxs.insert(idxs.end(),
                          {base, base + 1, base + 2, base, base + 2, base + 3});
            }

            // zero‐out mask
            for (int yy = 0; yy <= h; ++yy)
              for (int xx = 0; xx < w; ++xx)
                mask[(j + yy) * dims[u] + (i + xx)] = 0;

            i += w - 1; // advance x
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
