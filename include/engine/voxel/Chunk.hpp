
// include/engine/voxel/Chunk.hpp
#pragma once
#include "engine/render/Mesh.hpp"
#include "engine/render/Vertex.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

struct Block {
  bool solid = false; // simple for now; later you can add types/textures
};

class Chunk {
public:
  static constexpr int SIZE = 16; // 16x16x16 blocks

  Chunk(VkDevice device, VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool, VkQueue graphicsQueue);

  void generateMesh();

  Mesh *getMesh() const { return mesh_.get(); }

  Block &getBlock(int x, int y, int z) { return blocks_[x][y][z]; }

private:
  Block blocks_[SIZE][SIZE][SIZE];
  std::unique_ptr<Mesh> mesh_;

  // Vulkan handles
  VkDevice device_;
  VkPhysicalDevice physicalDevice_;
  VkCommandPool commandPool_;
  VkQueue graphicsQueue_;
};
