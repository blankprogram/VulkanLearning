
// include/engine/render/Mesh.hpp
#pragma once

#include "engine/render/Vertex.hpp"
#include <vector>
#include <vulkan/vulkan.h>

class Mesh {
public:
  Mesh(VkDevice device, VkPhysicalDevice physicalDevice,
       VkCommandPool commandPool, VkQueue graphicsQueue,
       const std::vector<Vertex> &vertices,
       const std::vector<uint32_t> &indices);
  ~Mesh();

  void Bind(VkCommandBuffer cmd) const;
  void Draw(VkCommandBuffer cmd) const;
  VkBuffer GetVertexBuffer() const { return vbo_; }
  VkBuffer GetIndexBuffer() const { return ibo_; }

private:
  VkDevice device_;
  VkBuffer vbo_;
  VkDeviceMemory vboMem_;
  VkBuffer ibo_;
  VkDeviceMemory iboMem_;
  uint32_t indexCount_;
};
