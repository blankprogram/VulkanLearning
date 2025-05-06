
// include/engine/scene/VoxelChunk.hpp
#pragma once

#include <cstdint>
#include <vector>

namespace engine {

// A node in the 4×4×4 TH‑octree
struct THNode {
  uint64_t occupancyMask; // one bit per child (4×4×4 = 64)
  int32_t childBaseIndex; // index of first child in flat array, or -1 if leaf
};

// A fixed‑size 3D voxel chunk with TH‑octree construction
class VoxelChunk {
public:
  VoxelChunk(int size);
  ~VoxelChunk() = default;

  // set or query voxels
  void setVoxel(int x, int y, int z, bool solid);
  bool isSolid(int x, int y, int z) const;

  // Build TH‑octree over this chunk, grouping bricks of brickDim³ voxels
  std::vector<THNode> buildTHOctree(int brickDim = 4) const;

private:
  int chunkSize_;               // width==height==depth
  std::vector<uint8_t> voxels_; // flattened 3D: 1=solid,0=empty

  // Recursively build at region origin (bx,by,bz) of size M, returns node index
  int buildRecursive(int bx, int by, int bz, int M, int brickDim,
                     std::vector<THNode> &nodes) const;
};

} // namespace engine
