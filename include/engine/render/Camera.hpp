#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace engine::render {

class Camera {
public:
  Camera(float fovY, float aspect, float nearPlane, float farPlane);

  void setPosition(const glm::vec3 &pos) { position = pos; }
  void setAspect(float a) { aspect = a; }
  void setRotation(float newYaw, float newPitch) {
    yaw = newYaw;

    // clamp pitch to just under ±90°
    constexpr float limit = glm::radians(89.0f);
    pitch = glm::clamp(newPitch, -limit, +limit);
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
    return glm::perspectiveRH_ZO(fovY, aspect, nearPlane, farPlane);
  }

  glm::mat4 viewProjection() const {
    // GLM's perspective flips Y by default, adjust if needed
    return projectionMatrix() * viewMatrix();
  }

  float getYaw() const { return yaw; }
  float getPitch() const { return pitch; }
  glm::vec3 front() const {
    return glm::normalize(
        glm::vec3{cos(pitch) * cos(yaw), sin(pitch), cos(pitch) * sin(yaw)});
  }
  glm::vec3 right() const {
    return glm::normalize(glm::cross(front(), glm::vec3{0, 1, 0}));
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
