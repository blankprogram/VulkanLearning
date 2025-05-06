#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;

  static vk::VertexInputBindingDescription getBindingDescription();
  static std::array<vk::VertexInputAttributeDescription, 2>
  getAttributeDescriptions();
};

} // namespace engine
