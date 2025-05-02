#pragma once
#include <glm/vec3.hpp>

namespace engine::voxel {
struct Voxel {
    bool solid = false;
    glm::vec3 color = glm::vec3(0.5f);
};
} // namespace engine::voxel
