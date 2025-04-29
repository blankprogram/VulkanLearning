#include "engine/render/Camera.hpp"

namespace engine::render {

Camera::Camera(float _fovY, float _aspect, float _nearPlane, float _farPlane)
    : fovY(_fovY), aspect(_aspect), nearPlane(_nearPlane), farPlane(_farPlane) {
}

} // namespace engine::render
