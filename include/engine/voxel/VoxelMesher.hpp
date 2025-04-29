#pragma once

#include "engine/render/Mesh.hpp"
#include "engine/voxel/VoxelVolume.hpp"
#include <memory>

namespace engine::voxel {

class VoxelMesher {
public:
  // Generates a mesh from volume data: one quad per solid->air face
  static std::unique_ptr<Mesh> GenerateMesh(const VoxelVolume &volume);
};

} // namespace engine::voxel
