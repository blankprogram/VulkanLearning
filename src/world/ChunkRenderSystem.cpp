

// src/world/ChunkRenderSystem.cpp
#include "engine/world/ChunkRenderSystem.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.h>

using namespace engine;
using namespace engine::world;

void ChunkRenderSystem::drawAll(RendererContext &ctx, const ChunkManager &mgr) {
  VkCommandBuffer cmdBuf = ctx.getCurrentCommandBuffer();
  VkPipelineLayout layout = ctx.getRenderResources().getPipeline().layout;

  for (const auto &[coordXZ, chunk] : mgr.getChunks()) {
    if (!chunk.mesh || chunk.mesh->indexCount() == 0)
      continue;

    glm::vec3 worldPos =
        glm::vec3(coordXZ.x * 16, 0.0f, coordXZ.y * 16); // CHUNK_DIM = 16
    glm::mat4 model = glm::translate(glm::mat4(1.0f), worldPos);

    vkCmdPushConstants(cmdBuf, layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(glm::mat4), &model);

    VkBuffer vbos[] = {chunk.mesh->vertexBuffer()};
    VkDeviceSize offs[] = {0};
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vbos, offs);
    vkCmdBindIndexBuffer(cmdBuf, chunk.mesh->indexBuffer(), 0,
                         VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmdBuf, static_cast<uint32_t>(chunk.mesh->indexCount()), 1,
                     0, 0, 0);
  }
}
