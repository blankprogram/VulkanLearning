#include "engine/render/Mesh.hpp"
#include "engine/utils/VulkanHelpers.hpp"

using engine::utils::CreateBuffer;

void Mesh::setVertices(std::vector<Vertex> &&v) { vertices_ = std::move(v); }

void Mesh::setIndices(std::vector<uint32_t> &&i) {
    indices_ = std::move(i);
    indices_count_ = indices_.size();
}
void Mesh::uploadToGPU(VulkanDevice *dev) {
    CreateBuffer(dev->getDevice(), dev->getPhysicalDevice(),
                 dev->getCommandPool(), dev->getGraphicsQueue(),
                 vertices_.data(), sizeof(Vertex) * vertices_.size(),
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vbo_, vboMem_);

    CreateBuffer(dev->getDevice(), dev->getPhysicalDevice(),
                 dev->getCommandPool(), dev->getGraphicsQueue(),
                 indices_.data(), sizeof(uint32_t) * indices_.size(),
                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT, ibo_, iboMem_);

    vertices_.clear();
    indices_.clear();
}
