
#pragma once

#include "engine/ecs/Components.hpp" // for Transform, MeshRef, MaterialRef
#include "engine/render/Pipeline.hpp"
#include <array>
#include <thirdparty/entt.hpp>
#include <vulkan/vulkan.h>

static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

class Renderer {
public:
  Renderer(VkDevice device, VkRenderPass renderPass,
           VkDescriptorSetLayout globalSetLayout, Pipeline &opaquePipeline,
           Pipeline &wireframePipeline);

  /// Draw all entities with Transform+MeshRef+MaterialRef
  void Render(
      entt::registry &registry, VkCommandBuffer cmd, VkExtent2D extent,
      const std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> &globalDescSets,
      uint32_t frameIndex, bool wireframe);

private:
  VkDevice device_;
  VkRenderPass renderPass_;
  VkDescriptorSetLayout globalSetLayout_;
  Pipeline &opaque_;
  Pipeline &wireframe_;
};
