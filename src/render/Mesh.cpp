
// src/render/Mesh.cpp
#include "engine/render/Mesh.hpp"
#include "engine/utils/VulkanHelpers.hpp" // assume CreateBuffer lives here

void Mesh::setVertices(std::vector<Vertex> &&v) { vertices_ = std::move(v); }
void Mesh::setIndices(std::vector<uint32_t> &&i) {
  indices_ = std::move(i);
  indices_count_ = indices_.size();
}

void Mesh::uploadToGPU(VulkanDevice *device) {
  // vertex buffer
  VulkanHelpers::CreateBuffer(
      device->device, device->physicalDevice, vertices_.data(),
      sizeof(Vertex) * vertices_.size(),
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      vbo_, vboMem_);

  // index buffer
  VulkanHelpers::CreateBuffer(
      device->device, device->physicalDevice, indices_.data(),
      sizeof(uint32_t) * indices_.size(),
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, ibo_,
      iboMem_);

  // free CPU arrays if desired
  vertices_.clear();
  indices_.clear();
}
