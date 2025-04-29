#include "engine/render/Mesh.hpp"
#include "engine/utils/VulkanHelpers.hpp"

using engine::utils::CreateBuffer;

void Mesh::uploadToGPU(VulkanDevice *dev) {
  // build GPU buffers via staging
  CreateBuffer(dev->device, dev->physicalDevice, dev->commandPool,
               dev->graphicsQueue, vertices_.data(),
               sizeof(Vertex) * vertices_.size(),
               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vbo_, vboMem_);

  CreateBuffer(dev->device, dev->physicalDevice, dev->commandPool,
               dev->graphicsQueue, indices_.data(),
               sizeof(uint32_t) * indices_.size(),
               VK_BUFFER_USAGE_INDEX_BUFFER_BIT, ibo_, iboMem_);

  vertices_.clear();
  indices_.clear();
}
