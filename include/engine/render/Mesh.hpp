#pragma once

#include "engine/render/Vertex.hpp"
#include <memory>
#include <vector>

class Mesh {
public:
  Mesh() = default;

  // Called by the mesher to populate CPU-side arrays
  void setVertices(std::vector<Vertex> &&verts);
  void setIndices(std::vector<uint32_t> &&idxs);

  // Later: upload vertex/index data to GPU
  void uploadToGPU();

  // Accessors for rendering
  const std::vector<Vertex> &vertices() const { return vertices_; }
  const std::vector<uint32_t> &indices() const { return indices_; }

private:
  std::vector<Vertex> vertices_;
  std::vector<uint32_t> indices_;
  // TODO: Vulkan buffer handles (VkBuffer, VkDeviceMemory) go here
};
