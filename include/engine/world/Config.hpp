#pragma once

#include <glm/ext/vector_int3.hpp>
namespace engine::world {

inline constexpr int VIEW_RADIUS = 16;

inline constexpr glm::ivec3 CHUNK_DIM = {16, 256, 16};

inline constexpr bool DEBUG = true;
} // namespace engine::world
