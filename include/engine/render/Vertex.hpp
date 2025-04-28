
#pragma once
#include <array>
#include <vulkan/vulkan.h>

struct Vertex {
  float pos[3];   // Now 3D: x, y, z
  float color[3]; // RGB color
  float uv[2];

  static VkVertexInputBindingDescription getBindingDesc() {
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(Vertex);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return binding;
  }

  static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescs() {
    std::array<VkVertexInputAttributeDescription, 3> attrs{};

    // location 0: position
    attrs[0].location = 0;
    attrs[0].binding = 0;
    attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[0].offset = offsetof(Vertex, pos);

    // location 1: color
    attrs[1].location = 1;
    attrs[1].binding = 0;
    attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrs[1].offset = offsetof(Vertex, color);

    // location 2: UV
    attrs[2].location = 2;
    attrs[2].binding = 0;
    attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
    attrs[2].offset = offsetof(Vertex, uv);

    return attrs;
  }
};
