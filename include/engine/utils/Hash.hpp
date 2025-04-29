
#pragma once
#include <functional>
#include <glm/glm.hpp>

namespace std {
template <> struct hash<glm::ivec3> {
  size_t operator()(const glm::ivec3 &v) const {
    size_t h1 = hash<int>()(v.x);
    size_t h2 = hash<int>()(v.y);
    size_t h3 = hash<int>()(v.z);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
  }
};
} // namespace std
