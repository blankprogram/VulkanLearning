#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;

  static vk::VertexInputBindingDescription getBindingDescription();
  static std::array<vk::VertexInputAttributeDescription, 3>
  getAttributeDescriptions();
};

} // namespace engine
