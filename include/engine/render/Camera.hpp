#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace engine::render {

class Camera {
public:
  Camera(float fovY, float aspect, float nearPlane, float farPlane);

  void setPosition(const glm::vec3 &pos) { position = pos; }
  void setRotation(float yawRadians, float pitchRadians) {
    yaw = yawRadians;
    pitch = pitchRadians;
  }

  const glm::vec3 &getPosition() const { return position; }

  glm::mat4 viewMatrix() const {
    glm::vec3 front;
    front.x = cos(pitch) * cos(yaw);
    front.y = sin(pitch);
    front.z = cos(pitch) * sin(yaw);
    return glm::lookAt(position, position + glm::normalize(front),
                       glm::vec3(0, 1, 0));
  }

  glm::mat4 projectionMatrix() const {
    return glm::perspective(fovY, aspect, nearPlane, farPlane);
  }

  glm::mat4 viewProjection() const {
    // GLM's perspective flips Y by default, adjust if needed
    return projectionMatrix() * viewMatrix();
  }

private:
  glm::vec3 position{0.0f, 0.0f, 0.0f};
  float yaw = glm::radians(-90.0f);
  float pitch = 0.0f;
  float fovY;
  float aspect;
  float nearPlane;
  float farPlane;
};

} // namespace engine::render
