
#pragma once
#include <array>
#include <vulkan/vulkan.h>

struct Vertex {
  float pos[3];
  float uv[2];

  static VkVertexInputBindingDescription getBindingDesc() {
    VkVertexInputBindingDescription b{};
    b.binding = 0;
    b.stride = sizeof(Vertex);
    b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return b;
  }

  static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescs() {
    std::array<VkVertexInputAttributeDescription, 2> a{};
    a[0].location = 0;
    a[0].binding = 0;
    a[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    a[0].offset = offsetof(Vertex, pos);

    a[1].location = 1;
    a[1].binding = 0;
    a[1].format = VK_FORMAT_R32G32_SFLOAT;
    a[1].offset = offsetof(Vertex, uv);
    return a;
  }
};
