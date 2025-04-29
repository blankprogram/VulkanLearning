
#pragma once
#include "OctreeNode.hpp"
#include "engine/render/Vertex.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class VoxelVolume {
public:
  void insert(const glm::ivec3 &pos, const Voxel &voxel);
  const Voxel *get(const glm::ivec3 &pos) const;

  void generateMesh(std::vector<Vertex> &outVertices,
                    std::vector<uint32_t> &outIndices);

private:
  std::unique_ptr<OctreeNode> root_ = std::make_unique<OctreeNode>();
  void insertRecursive(OctreeNode *node, glm::ivec3 pos, int depth,
                       const Voxel &voxel);
  const Voxel *getRecursive(const OctreeNode *node, glm::ivec3 pos,
                            int depth) const;
};
