
// Vertex.h
#pragma once
#include <array>
#include <vulkan/vulkan.h>

// Simple 2D pos + RGB color vertex
struct Vertex {
  float pos[2];
  float color[3];

  static VkVertexInputBindingDescription getBindingDesc() {
    VkVertexInputBindingDescription bd{};
    bd.binding = 0;
    bd.stride = sizeof(Vertex);
    bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bd;
  }

  static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescs() {
    std::array<VkVertexInputAttributeDescription, 2> ads{};
    // location 0 = pos (vec2)
    ads[0].location = 0;
    ads[0].binding = 0;
    ads[0].format = VK_FORMAT_R32G32_SFLOAT;
    ads[0].offset = offsetof(Vertex, pos);
    // location 1 = color (vec3)
    ads[1].location = 1;
    ads[1].binding = 0;
    ads[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    ads[1].offset = offsetof(Vertex, color);
    return ads;
  }
};
