
#pragma once

#include <glm/glm.hpp>

namespace engine {

/// Simple free‑fly camera: position + yaw/pitch
class Camera {
public:
  /// Construct at given world‑space position, yaw (° around Z), pitch (°
  /// up/down)
  Camera(glm::vec3 pos = {0.0f, 0.0f, 5.0f}, float yaw = -90.0f,
         float pitch = 0.0f);

  /// Get view matrix (to put into UBO.view)
  glm::mat4 getViewMatrix() const;

  /// Move camera in local axes: x = right, y = up, z = forward
  void processKeyboard(const glm::vec3 &delta);

  /// Rotate camera: deltaX = yaw change, deltaY = pitch change
  void processMouse(float deltaX, float deltaY);

private:
  void updateVectors();

  glm::vec3 position_;
  float yaw_, pitch_;

  glm::vec3 front_;
  glm::vec3 up_;
  glm::vec3 right_;
  const glm::vec3 worldUp_{0.0f, 0.0f, 1.0f};
};

} // namespace engine
