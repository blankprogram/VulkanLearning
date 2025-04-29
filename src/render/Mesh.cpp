
#include "engine/render/Mesh.hpp"

void Mesh::setVertices(std::vector<Vertex> &&verts) {
  vertices_ = std::move(verts);
}

void Mesh::setIndices(std::vector<uint32_t> &&idxs) {
  indices_ = std::move(idxs);
}

void Mesh::uploadToGPU() {
  // TODO:
  // 1) create Vulkan vertex buffer from vertices_
  // 2) create Vulkan index buffer from indices_
  // 3) optionally free CPU-side arrays
}
