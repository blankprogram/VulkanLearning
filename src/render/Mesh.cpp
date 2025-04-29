#include "engine/render/Mesh.hpp"
#include "engine/utils/VulkanHelpers.hpp" // createBuffer()/copyBuffer()
#include <cstring>                        // std::memcpy
#include <vector>

Mesh::Mesh(VkDevice device, VkPhysicalDevice physicalDevice,
           VkCommandPool commandPool, VkQueue graphicsQueue,
           const std::vector<Vertex> &vertices,
           const std::vector<uint32_t> &indices)
    : device_(device), indexCount_(static_cast<uint32_t>(indices.size())) {

  if (vertices.empty() || indices.empty()) {
    vbo_ = VK_NULL_HANDLE;
    vboMem_ = VK_NULL_HANDLE;
    ibo_ = VK_NULL_HANDLE;
    iboMem_ = VK_NULL_HANDLE;
    return;
  }

  VkDeviceSize vboSize = sizeof(vertices[0]) * vertices.size();
  VkDeviceSize iboSize = sizeof(indices[0]) * indices.size();

  // Create staging buffers
  VkBuffer stagingVB;
  VkDeviceMemory stagingVBmem;
  VkBuffer stagingIB;
  VkDeviceMemory stagingIBmem;

  createBuffer(device_, physicalDevice, vboSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingVB, stagingVBmem);
  createBuffer(device_, physicalDevice, iboSize,
               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingIB, stagingIBmem);

  // Upload vertex data
  void *data;
  vkMapMemory(device_, stagingVBmem, 0, vboSize, 0, &data);
  std::memcpy(data, vertices.data(), static_cast<size_t>(vboSize));
  vkUnmapMemory(device_, stagingVBmem);

  // Upload index data
  vkMapMemory(device_, stagingIBmem, 0, iboSize, 0, &data);
  std::memcpy(data, indices.data(), static_cast<size_t>(iboSize));
  vkUnmapMemory(device_, stagingIBmem);

  // Create device-local buffers
  createBuffer(device_, physicalDevice, vboSize,
               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                   VK_BUFFER_USAGE_TRANSFER_DST_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vbo_, vboMem_);
  createBuffer(device_, physicalDevice, iboSize,
               VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                   VK_BUFFER_USAGE_TRANSFER_DST_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ibo_, iboMem_);

  // Copy staging -> device-local
  copyBuffer(device_, commandPool, graphicsQueue, stagingVB, vbo_, vboSize);
  copyBuffer(device_, commandPool, graphicsQueue, stagingIB, ibo_, iboSize);

  // Clean up staging
  vkDestroyBuffer(device_, stagingVB, nullptr);
  vkFreeMemory(device_, stagingVBmem, nullptr);
  vkDestroyBuffer(device_, stagingIB, nullptr);
  vkFreeMemory(device_, stagingIBmem, nullptr);
}

Mesh::~Mesh() {
  vkDestroyBuffer(device_, vbo_, nullptr);
  vkFreeMemory(device_, vboMem_, nullptr);
  vkDestroyBuffer(device_, ibo_, nullptr);
  vkFreeMemory(device_, iboMem_, nullptr);
}

void Mesh::Bind(VkCommandBuffer cmd) const {
  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(cmd, 0, 1, &vbo_, &offset);
  vkCmdBindIndexBuffer(cmd, ibo_, 0, VK_INDEX_TYPE_UINT32);
}

void Mesh::Draw(VkCommandBuffer cmd) const {
  vkCmdDrawIndexed(cmd, indexCount_, 1, 0, 0, 0);
}
