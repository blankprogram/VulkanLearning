
// include/engine/ecs/Components.hpp
#pragma once
#include "../render/Material.hpp"
#include "../render/Mesh.hpp"
#include "../scene/Camera.hpp"
#include <glm/glm.hpp>

struct Transform {
  glm::mat4 model{};
};
struct MeshRef {
  Mesh *mesh = nullptr;
};
struct MaterialRef {
  Material *mat = nullptr;
};

struct CameraComponent {
  Camera cam; //< your glm lookAt / perspective
};
