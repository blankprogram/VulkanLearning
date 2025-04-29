
#pragma once
#include "engine/render/Camera.hpp"
#include <GLFW/glfw3.h>

class InputManager {
public:
  InputManager(GLFWwindow *window, engine::render::Camera &cam);

  // call once per-frame, passing dt in seconds
  void processInput(float dt);

private:
  static void mouseCallback(GLFWwindow *w, double xpos, double ypos);

  GLFWwindow *window_;
  engine::render::Camera &cam_;

  bool firstMouse_ = true;
  float lastX_ = 0.f, lastY_ = 0.f;

  // camera control params
  float moveSpeed_ = 10.f;    // units/sec
  float sprintMul_ = 4.f;     // when shift held
  float mouseSens_ = 0.0025f; // radians per pixel
};
