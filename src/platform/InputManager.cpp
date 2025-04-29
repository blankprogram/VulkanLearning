
#include "engine/platform/InputManager.hpp"
#include <GLFW/glfw3.h>

InputManager::InputManager(GLFWwindow *window, engine::render::Camera &cam)
    : window_(window), cam_(cam) {
  // hide & lock the cursor
  glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  // set our static callback
  glfwSetCursorPosCallback(window_, &InputManager::mouseCallback);
  // associate this instance with the window so callback can find it
  glfwSetWindowUserPointer(window_, this);
}

void InputManager::mouseCallback(GLFWwindow *w, double xpos, double ypos) {
  auto *self = reinterpret_cast<InputManager *>(glfwGetWindowUserPointer(w));
  if (self->firstMouse_) {
    self->lastX_ = float(xpos);
    self->lastY_ = float(ypos);
    self->firstMouse_ = false;
  }

  float dx = float(xpos) - self->lastX_;
  float dy = float(self->lastY_) - float(ypos);
  self->lastX_ = float(xpos);
  self->lastY_ = float(ypos);

  // update camera yaw/pitch
  float yaw = dx * self->mouseSens_;
  float pitch = dy * self->mouseSens_;
  // accumulate
  self->cam_.setRotation(self->cam_.getYaw() + yaw,
                         self->cam_.getPitch() + pitch);
}

void InputManager::processInput(float dt) {
  // WASD movement
  glm::vec3 forward = cam_.front(); // you’ll need to expose these in Camera
  glm::vec3 right = cam_.right();
  glm::vec3 up = glm::vec3(0, 1, 0);

  float speed =
      moveSpeed_ * (glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
                        ? sprintMul_
                        : 1.f);

  glm::vec3 delta{0};
  if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS)
    delta += forward;
  if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS)
    delta -= forward;
  if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS)
    delta -= right;
  if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS)
    delta += right;
  if (glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS)
    delta += up;
  if (glfwGetKey(window_, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    delta -= up;

  if (glm::length(delta) > 0.001f) {
    delta = glm::normalize(delta) * speed * dt;
    cam_.setPosition(cam_.getPosition() + delta);
  }
}
