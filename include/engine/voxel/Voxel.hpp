
#pragma once
#include <glm/glm.hpp>

struct Voxel {
  glm::vec3 color{1.0f}; // Default: white
  float density = 1.0f;  // For soft voxels, unused for solid
  bool solid = true;     // For now, treat solid as occupied
};
