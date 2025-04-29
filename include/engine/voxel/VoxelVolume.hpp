
#pragma once

#include <glm/vec3.hpp>
#include <vector>

namespace engine::voxel {

struct Voxel {
  bool solid = false;
  // you can expand this later with color, material ID, etc.
};

class VoxelVolume {
public:
  // Constructs a volume at chunk-coords origin with given dimensions
  VoxelVolume(const glm::ivec3 &extent);

  // Read/write access
  Voxel &at(int x, int y, int z);
  const Voxel &at(int x, int y, int z) const;

  // Total dimensions in voxels
  glm::ivec3 extent;

private:
  std::vector<Voxel> data_;
  inline size_t index(int x, int y, int z) const {
    return (z * extent.y + y) * extent.x + x;
  }
};

} // namespace engine::voxel
