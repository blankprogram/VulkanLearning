
// include/engine/scene/Camera.hpp
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/// keys for ProcessKeyboard()
enum CameraMovement { FORWARD, BACKWARD, LEFT, RIGHT };

class Camera {
public:
  /// ctor: supply your aspect or use defaults
  Camera(float aspect, glm::vec3 position = {5.0f, 5.0f, 2.0f},
         glm::vec3 up = {0.0f, 1.0f, 0.0f}, float yaw = -90.0f,
         float pitch = 0.0f);

  /// must call on resize
  void SetAspect(float aspect);

  /// WASD movement
  void ProcessKeyboard(CameraMovement direction, float deltaTime);

  /// mouse-look
  void ProcessMouseMovement(float xoffset, float yoffset,
                            bool constrainPitch = true);

  /// scroll-wheel zoom
  void ProcessMouseScroll(float yoffset);

  const glm::vec3 &GetPosition() const { return Position; }
  /// what you bind into your UBO
  const glm::mat4 &GetView() const { return view_; }
  const glm::mat4 &GetProj() const { return proj_; }

private:
  void updateCameraVectors();
  void updateViewMatrix();
  void updateProjMatrix();

  // camera state
  glm::vec3 Position, Front, Up, Right, WorldUp;
  float Yaw, Pitch;

  // options
  float MovementSpeed = 10.0f;
  float MouseSensitivity = 0.1f;
  float Zoom = 45.0f; // fovY in degrees

  // projection params
  float Aspect;
  float ZNear = 0.1f, ZFar = 100.0f;

  // cached matrices
  glm::mat4 view_{1.0f};
  glm::mat4 proj_{1.0f};
};
