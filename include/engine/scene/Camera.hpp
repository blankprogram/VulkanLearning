
// include/engine/scene/Camera.hpp
#pragma once
#include <glm/glm.hpp>

class Camera {
public:
  Camera(float aspect);
  void SetAspect(float aspect);
  void LookAt(const glm::vec3 &eye, const glm::vec3 &center,
              const glm::vec3 &up);
  const glm::mat4 &GetView() const;
  const glm::mat4 &GetProj() const;

private:
  void RecalcProj();
  void RecalcView();

  float aspect_, fovY_ = glm::radians(45.0f), zNear_ = 0.1f, zFar_ = 10.0f;
  glm::vec3 eye_{2, 2, 2}, center_{0, 0, 0}, up_{0, 0, 1};
  glm::mat4 view_{1.0f}, proj_{1.0f};
};
