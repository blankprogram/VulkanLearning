
// src/scene/Camera.cpp
#include "engine/scene/Camera.hpp"

Camera::Camera(float aspect, glm::vec3 position, glm::vec3 up, float yaw,
               float pitch)
    : Position(position), WorldUp(up), Yaw(yaw), Pitch(pitch), Aspect(aspect) {
  // initialize Front, Right, Up
  Front = glm::vec3(0.0f, 0.0f, -1.0f);
  updateCameraVectors();
  updateProjMatrix();
  updateViewMatrix();
}

void Camera::SetAspect(float aspect) {
  Aspect = aspect;
  updateProjMatrix();
}

void Camera::ProcessKeyboard(CameraMovement dir, float dt) {
  float velocity = MovementSpeed * dt;
  if (dir == FORWARD)
    Position += Front * velocity;
  if (dir == BACKWARD)
    Position -= Front * velocity;
  if (dir == LEFT)
    Position -= Right * velocity;
  if (dir == RIGHT)
    Position += Right * velocity;
  updateViewMatrix();
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset,
                                  bool constrainPitch) {
  xoffset *= MouseSensitivity;
  yoffset *= MouseSensitivity;

  Yaw += xoffset;
  Pitch += yoffset;
  if (constrainPitch) {
    if (Pitch > 89.0f)
      Pitch = 89.0f;
    if (Pitch < -89.0f)
      Pitch = -89.0f;
  }

  updateCameraVectors();
  updateViewMatrix();
}

void Camera::ProcessMouseScroll(float yoffset) {
  Zoom -= yoffset;
  if (Zoom < 1.0f)
    Zoom = 1.0f;
  if (Zoom > 45.0f)
    Zoom = 45.0f;
  updateProjMatrix();
}

void Camera::updateCameraVectors() {
  glm::vec3 front;
  front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
  front.y = sin(glm::radians(Pitch));
  front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
  Front = glm::normalize(front);

  Right = glm::normalize(glm::cross(Front, WorldUp));
  Up = glm::normalize(glm::cross(Right, Front));
}

void Camera::updateViewMatrix() {
  view_ = glm::lookAt(Position, Position + Front, Up);
}

void Camera::updateProjMatrix() {
  proj_ = glm::perspective(glm::radians(Zoom), Aspect, ZNear, ZFar);
  proj_[1][1] *= -1;
}
