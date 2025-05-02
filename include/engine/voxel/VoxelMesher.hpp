#pragma once

#include "engine/render/Mesh.hpp"
#include "engine/voxel/VoxelVolume.hpp"
#include <memory>

namespace engine::voxel {

class VoxelMesher {
  public:
    static std::unique_ptr<Mesh> GenerateMesh(const VoxelVolume &volume);
};

} // namespace engine::voxel
