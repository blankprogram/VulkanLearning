
// src/scene/VoxelChunk.cpp
#include "engine/scene/VoxelChunk.hpp"
#include "engine/voxel/VoxelChunk.hpp"
#include <functional>
#include <stdexcept>
#include <vector>

namespace engine {

VoxelChunk::VoxelChunk(int size)
    : chunkSize_(size), voxels_(size * size * size, 0) {}

void VoxelChunk::setVoxel(int x, int y, int z, bool solid) {
  if (x < 0 || y < 0 || z < 0 || x >= chunkSize_ || y >= chunkSize_ ||
      z >= chunkSize_)
    throw std::out_of_range("VoxelChunk::setVoxel");
  voxels_[(z * chunkSize_ + y) * chunkSize_ + x] = solid ? 1 : 0;
}

bool VoxelChunk::isSolid(int x, int y, int z) const {
  if (x < 0 || y < 0 || z < 0 || x >= chunkSize_ || y >= chunkSize_ ||
      z >= chunkSize_)
    return false;
  return voxels_[(z * chunkSize_ + y) * chunkSize_ + x] != 0;
}

std::vector<THNode> VoxelChunk::buildTHOctree(int brickDim) const {
  std::vector<THNode> nodes;
  nodes.reserve(1);

  // recursive helper lambda
  std::function<int(int, int, int, int)> buildRecursive =
      [&](int ox, int oy, int oz, int size) -> int {
    int myIndex = int(nodes.size());
    nodes.push_back({0, -1});
    uint64_t mask = 0;

    if (size == brickDim) {
      // leaf: one bit per voxel in this brick
      int bit = 0;
      for (int z = 0; z < brickDim; ++z)
        for (int y = 0; y < brickDim; ++y)
          for (int x = 0; x < brickDim; ++x) {
            if (isSolid(ox + x, oy + y, oz + z))
              mask |= (uint64_t(1) << bit);
            ++bit;
          }
    } else {
      // interior: group into brickDim^3 sub‑bricks
      int childSize = size / brickDim;
      int bit = 0;
      // first pass: compute mask
      for (int cz = 0; cz < brickDim; ++cz)
        for (int cy = 0; cy < brickDim; ++cy)
          for (int cx = 0; cx < brickDim; ++cx) {
            bool any = false;
            int sx = ox + cx * childSize, sy = oy + cy * childSize,
                sz = oz + cz * childSize;
            for (int z = 0; z < childSize && !any; ++z)
              for (int y = 0; y < childSize && !any; ++y)
                for (int x = 0; x < childSize; ++x)
                  if (isSolid(sx + x, sy + y, sz + z)) {
                    any = true;
                    break;
                  }
            if (any)
              mask |= (uint64_t(1) << bit);
            ++bit;
          }
      if (mask) {
        nodes[myIndex].childBaseIndex = int(nodes.size());
        bit = 0;
        for (int cz = 0; cz < brickDim; ++cz)
          for (int cy = 0; cy < brickDim; ++cy)
            for (int cx = 0; cx < brickDim; ++cx) {
              if (mask & (uint64_t(1) << bit)) {
                int sx = ox + cx * childSize, sy = oy + cy * childSize,
                    sz = oz + cz * childSize;
                buildRecursive(sx, sy, sz, childSize);
              }
              ++bit;
            }
      }
    }

    nodes[myIndex].occupancyMask = mask;
    return myIndex;
  };

  // start at origin, full chunk
  buildRecursive(0, 0, 0, chunkSize_);
  return nodes;
}

} // namespace engine
