
#include "engine/voxel/VoxelVolume.hpp"

void VoxelVolume::insert(const glm::ivec3 &pos, const Voxel &voxel) {
  insertRecursive(root_.get(), pos, 7, voxel); // depth 5 = 32³ volume
}

void VoxelVolume::insertRecursive(OctreeNode *node, glm::ivec3 pos, int depth,
                                  const Voxel &voxel) {
  if (depth == 0) {
    node->isLeaf = true;
    node->voxel = voxel;
    return;
  }

  int size = 1 << (depth - 1); // half-size of current node
  int index = 0;
  if (pos.x >= size) {
    index |= 1;
    pos.x -= size;
  }
  if (pos.y >= size) {
    index |= 2;
    pos.y -= size;
  }
  if (pos.z >= size) {
    index |= 4;
    pos.z -= size;
  }

  if (!node->children[index])
    node->children[index] = std::make_unique<OctreeNode>();

  insertRecursive(node->children[index].get(), pos, depth - 1, voxel);
}

const Voxel *VoxelVolume::get(const glm::ivec3 &pos) const {
  return getRecursive(root_.get(), pos, 7);
}

const Voxel *VoxelVolume::getRecursive(const OctreeNode *node, glm::ivec3 pos,
                                       int depth) const {
  if (!node)
    return nullptr;

  if (node->isLeaf)
    return node->voxel ? &(*node->voxel) : nullptr;

  int size = 1 << (depth - 1);
  int index = 0;
  if (pos.x >= size) {
    index |= 1;
    pos.x -= size;
  }
  if (pos.y >= size) {
    index |= 2;
    pos.y -= size;
  }
  if (pos.z >= size) {
    index |= 4;
    pos.z -= size;
  }

  return getRecursive(node->children[index].get(), pos, depth - 1);
}

// Simple greedy meshing stub (not optimized)
void VoxelVolume::generateMesh(std::vector<Vertex> &outVertices,
                               std::vector<uint32_t> &outIndices) {
  outVertices.clear();
  outIndices.clear();
  int ChunkSize = 128;
  for (int x = 0; x < ChunkSize; ++x)
    for (int y = 0; y < ChunkSize; ++y)
      for (int z = 0; z < ChunkSize; ++z) {
        glm::ivec3 pos(x, y, z);
        const Voxel *v = get(pos);
        if (v && v->solid) {
          const float voxelScale = 0.25f; // make each voxel 0.25 units

          glm::vec3 p = glm::vec3(pos) * voxelScale;
          glm::vec3 c = v->color;

          const float voxelSize = 0.25f; // <<< real voxel cube size now

          auto makeFace = [&](glm::vec3 offset1, glm::vec3 offset2,
                              glm::vec3 offset3, glm::vec3 offset4) {
            uint32_t base = static_cast<uint32_t>(outVertices.size());
            outVertices.push_back(Vertex{{(p + offset1 * voxelSize).x,
                                          (p + offset1 * voxelSize).y,
                                          (p + offset1 * voxelSize).z},
                                         {0, 0},
                                         {c.x, c.y, c.z}});
            outVertices.push_back(Vertex{{(p + offset2 * voxelSize).x,
                                          (p + offset2 * voxelSize).y,
                                          (p + offset2 * voxelSize).z},
                                         {1, 0},
                                         {c.x, c.y, c.z}});
            outVertices.push_back(Vertex{{(p + offset3 * voxelSize).x,
                                          (p + offset3 * voxelSize).y,
                                          (p + offset3 * voxelSize).z},
                                         {1, 1},
                                         {c.x, c.y, c.z}});
            outVertices.push_back(Vertex{{(p + offset4 * voxelSize).x,
                                          (p + offset4 * voxelSize).y,
                                          (p + offset4 * voxelSize).z},
                                         {0, 1},
                                         {c.x, c.y, c.z}});
            outIndices.insert(outIndices.end(), {base, base + 1, base + 2,
                                                 base + 2, base + 3, base});
          };

          if (!get({x, y, z + 1})) { // +Z
            makeFace({0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1});
          }
          if (!get({x, y, z - 1})) { // -Z
            makeFace({0, 1, 0}, {1, 1, 0}, {1, 0, 0}, {0, 0, 0});
          }
          if (!get({x + 1, y, z})) { // +X
            makeFace({1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1});
          }
          if (!get({x - 1, y, z})) { // -X
            makeFace({0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0});
          }
          if (!get({x, y + 1, z})) { // +Y
            makeFace({0, 1, 1}, {1, 1, 1}, {1, 1, 0}, {0, 1, 0});
          }
          if (!get({x, y - 1, z})) { // -Y
            makeFace({0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1});
          }
        }
      }
}
