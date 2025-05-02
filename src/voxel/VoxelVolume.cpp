#include "engine/voxel/VoxelVolume.hpp"
#include <stdexcept>

using namespace engine::voxel;

VoxelVolume::VoxelVolume(const glm::ivec3 &ext)
    : extent(ext), data_(ext.x * ext.y * ext.z) {}

Voxel &VoxelVolume::at(int x, int y, int z) {
    if (x < 0 || y < 0 || z < 0 || x >= extent.x || y >= extent.y ||
        z >= extent.z)
        throw std::out_of_range("VoxelVolume::at coords");
    return data_[index(x, y, z)];
}

const Voxel &VoxelVolume::at(int x, int y, int z) const {
    if (x < 0 || y < 0 || z < 0 || x >= extent.x || y >= extent.y ||
        z >= extent.z)
        throw std::out_of_range("VoxelVolume::at coords");
    return data_[index(x, y, z)];
}
