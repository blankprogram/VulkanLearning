
#include "engine/render/Renderer.hpp"
#include <glm/gtc/type_ptr.hpp> // for glm::value_ptr

Renderer::Renderer(VkDevice device, VkRenderPass renderPass,
                   VkDescriptorSetLayout globalSetLayout,
                   Pipeline &opaquePipeline, Pipeline &wireframePipeline)
    : device_(device), renderPass_(renderPass),
      globalSetLayout_(globalSetLayout), opaque_(opaquePipeline),
      wireframe_(wireframePipeline) {}

void Renderer::Render(
    entt::registry &registry, VkCommandBuffer cmd, VkExtent2D extent,
    const std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> &globalDescSets,
    uint32_t frameIndex, bool wireframe) {
  // Choose which pipeline to bind
  auto &pipeline = wireframe ? wireframe_ : opaque_;

  // 1) Bind pipeline & per-frame UBO (set = 0)
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline.GetPipeline());
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          pipeline.GetLayout(),
                          0,                           // firstSet
                          1,                           // descriptorSetCount
                          &globalDescSets[frameIndex], // pDescriptorSets
                          0, nullptr                   // dynamic offsets
  );

  // 2) Set dynamic viewport & scissor
  VkViewport vp{0.0f, 0.0f, float(extent.width), float(extent.height),
                0.0f, 1.0f};
  vkCmdSetViewport(cmd, 0, 1, &vp);
  VkRect2D scissor{{0, 0}, extent};
  vkCmdSetScissor(cmd, 0, 1, &scissor);

  // 3) Draw all entities with Transform + MeshRef + MaterialRef
  auto view = registry.view<Transform, MeshRef, MaterialRef>();
  for (auto e : view) {
    auto &tf = view.get<Transform>(e);
    auto &mesh = view.get<MeshRef>(e).mesh;
    auto &mat = view.get<MaterialRef>(e).mat;

    // 3a) Push-constants (model matrix)
    vkCmdPushConstants(cmd, pipeline.GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(glm::mat4), glm::value_ptr(tf.model));

    // 3b) Bind this mesh’s vertex/index buffers
    mesh->Bind(cmd);

    // 3c) Bind this material’s descriptor set (set = 1)
    mat->Bind(cmd, pipeline.GetLayout(), /*setIndex=*/1);

    // 3d) Draw it
    mesh->Draw(cmd);
  }
}
