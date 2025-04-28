
// src/scene/Camera.cpp
#include "engine/scene/Camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(float aspect) : aspect_(aspect) {
  RecalcProj();
  RecalcView();
}

void Camera::SetAspect(float aspect) {
  aspect_ = aspect;
  RecalcProj();
}

void Camera::LookAt(const glm::vec3 &eye_, const glm::vec3 &center_,
                    const glm::vec3 &up_) {
  this->eye_ = eye_;
  this->center_ = center_;
  this->up_ = up_;
  RecalcView();
}

const glm::mat4 &Camera::GetView() const { return view_; }
const glm::mat4 &Camera::GetProj() const { return proj_; }

void Camera::RecalcProj() {
  proj_ = glm::perspective(fovY_, aspect_, zNear_, zFar_);
  proj_[1][1] *= -1;
}

void Camera::RecalcView() { view_ = glm::lookAt(eye_, center_, up_); }
