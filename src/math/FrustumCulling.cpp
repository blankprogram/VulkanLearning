
#include "engine/math/FrustumCulling.hpp"
#include <glm/gtc/matrix_access.hpp> // for glm::transpose

using namespace engine::math;

glm::vec4 FrustumCuller::normalizePlane(const glm::vec4 &p) {
  float invLen = 1.0f / glm::length(glm::vec3(p));
  return p * invLen;
}

void FrustumCuller::update(const glm::mat4 &m) {
  // transpose so that we can treat t[i] as the original row i
  glm::mat4 t = glm::transpose(m);

  planes[0] = normalizePlane(t[3] + t[0]); // left
  planes[1] = normalizePlane(t[3] - t[0]); // right
  planes[2] = normalizePlane(t[3] + t[1]); // bottom
  planes[3] = normalizePlane(t[3] - t[1]); // top
  planes[4] = normalizePlane(t[3] + t[2]); // near
  planes[5] = normalizePlane(t[3] - t[2]); // far
}

bool FrustumCuller::isBoxVisible(const glm::vec3 &mn,
                                 const glm::vec3 &mx) const {
  for (int p = 0; p < 6; ++p) {
    const glm::vec4 &pl = planes[p];
    glm::vec3 negative = {pl.x > 0 ? mn.x : mx.x, pl.y > 0 ? mn.y : mx.y,
                          pl.z > 0 ? mn.z : mx.z};
    if (glm::dot(glm::vec3(pl), negative) + pl.w < 0.0f)
      return false;
  }
  return true;
}
