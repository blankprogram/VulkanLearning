
#pragma once

#include <glm/glm.hpp>

namespace engine::math {

/// A simple view‑frustum built from a viewProj matrix.
/// Planes are in the form (a,b,c,d) where ax+by+cz+d=0.
class FrustumCuller {
public:
  void update(const glm::mat4 &viewProj);

  bool isBoxVisible(const glm::vec3 &mn, const glm::vec3 &mx) const;

private:
  glm::vec4 planes[6];

  static glm::vec4 normalizePlane(const glm::vec4 &p);
};

} // namespace engine::math
