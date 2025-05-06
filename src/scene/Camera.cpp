
#include "engine/scene/Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace engine {

Camera::Camera(glm::vec3 pos, float yaw, float pitch)
    : position_(pos), yaw_(yaw), pitch_(pitch) {
  updateVectors();
}

glm::mat4 Camera::getViewMatrix() const {
  // look from position_ toward position_+front_, with up_ vector
  return glm::lookAt(position_, position_ + front_, up_);
}

void Camera::processKeyboard(const glm::vec3 &delta) {
  // move in local camera axes
  position_ += delta.x * right_;
  position_ += delta.y * up_;
  position_ += delta.z * front_;
}

void Camera::processMouse(float deltaX, float deltaY) {
  constexpr float sensitivity = 0.1f;
  yaw_ += deltaX * sensitivity;
  pitch_ += deltaY * sensitivity;

  // constrain pitch
  if (pitch_ > 89.0f)
    pitch_ = 89.0f;
  if (pitch_ < -89.0f)
    pitch_ = -89.0f;

  updateVectors();
}

void Camera::updateVectors() {
  // convert spherical to cartesian
  glm::vec3 f;
  f.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
  f.y = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
  f.z = sin(glm::radians(pitch_));
  front_ = glm::normalize(f);

  // re‑compute right and up
  right_ = glm::normalize(glm::cross(front_, worldUp_));
  up_ = glm::normalize(glm::cross(right_, front_));
}

} // namespace engine
