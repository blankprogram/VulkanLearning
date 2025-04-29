
// src/world/ChunkRenderSystem.cpp
#include "engine/world/ChunkRenderSystem.hpp"
#include "engine/math/FrustumCulling.hpp"
#include <glm/glm.hpp>     // for any transforms
#include <vulkan/vulkan.h> // for vkCmd*

using namespace engine;
using namespace engine::world;

void ChunkRenderSystem::drawAll(RendererContext &ctx, const ChunkManager &mgr) {
  VkCommandBuffer cmdBuf = ctx.getCurrentCommandBuffer();

  // Iterate only via public getter
  for (auto const &[coord, chunk] : mgr.getChunks()) {
    if (!chunk.mesh)
      continue;

    size_t ic = chunk.mesh->indexCount();
    if (ic == 0)
      continue;

    // If you want real culling, supply transform & bounds here:
    // if (!FrustumCulling::BoxVisible(transform, bounds)) continue;
    // for now, skip culling

    // bind vertex/index
    VkBuffer vbos[] = {chunk.mesh->vertexBuffer()};
    VkDeviceSize offs[] = {0};
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vbos, offs);
    vkCmdBindIndexBuffer(cmdBuf, chunk.mesh->indexBuffer(), 0,
                         VK_INDEX_TYPE_UINT32);

    // bind your pipeline, descriptor sets, push constants, etc.

    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(ic), 1, 0, 0, 0);
  }
}
