
// include/engine/render/Mesh.hpp
#pragma once

#include "engine/platform/VulkanDevice.hpp"
#include "engine/render/Vertex.hpp"
#include <vector>

class Mesh {
public:
  Mesh() = default;

  void setVertices(std::vector<Vertex> &&v);
  void setIndices(std::vector<uint32_t> &&i);

  // upload to GPU
  void uploadToGPU(VulkanDevice *device);

  // getters for binding
  VkBuffer vertexBuffer() const { return vbo_; }
  VkBuffer indexBuffer() const { return ibo_; }
  size_t indexCount() const { return indices_count_; }

private:
  std::vector<Vertex> vertices_;
  std::vector<uint32_t> indices_;
  // Vulkan handles
  VkBuffer vbo_ = VK_NULL_HANDLE;
  VkDeviceMemory vboMem_ = VK_NULL_HANDLE;
  VkBuffer ibo_ = VK_NULL_HANDLE;
  VkDeviceMemory iboMem_ = VK_NULL_HANDLE;
  size_t indices_count_ = 0;
};
