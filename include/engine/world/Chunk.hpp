#pragma once

#include "engine/render/Mesh.hpp"
#include "engine/voxel/VoxelVolume.hpp"
#include <glm/vec2.hpp>
#include <memory>

namespace engine::world {

struct Chunk {
    glm::ivec2 coord;
    std::unique_ptr<engine::voxel::VoxelVolume> volume;
    std::unique_ptr<Mesh> mesh;
    bool dirty = true;
    bool meshJobQueued = false;
};

} // namespace engine::world
