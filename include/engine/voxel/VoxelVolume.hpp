
#pragma once
#include "engine/voxel/Voxel.hpp"
#include <glm/vec3.hpp>
#include <vector>

namespace engine::voxel {
class VoxelVolume {
public:
  VoxelVolume(const glm::ivec3 &extent);
  Voxel &at(int x, int y, int z);
  const Voxel &at(int x, int y, int z) const;
  glm::ivec3 extent;

private:
  std::vector<Voxel> data_;
  size_t index(int x, int y, int z) const {
    return (z * extent.y + y) * extent.x + x;
  }
};
} // namespace engine::voxel
