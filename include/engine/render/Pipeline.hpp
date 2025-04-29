
#pragma once

#include <string>
#include <vulkan/vulkan.h>

namespace engine::render {

struct Pipeline {
  VkPipeline pipeline{VK_NULL_HANDLE};
  VkPipelineLayout layout{VK_NULL_HANDLE};

  /// @param device       logical Vulkan device
  /// @param renderPass   compatible render pass
  /// @param dsl          descriptor‐set layout for UBOs/textures
  /// @param vertSPV      file path to vertex shader SPIR-V
  /// @param fragSPV      file path to fragment shader SPIR-V
  void init(VkDevice device, VkRenderPass renderPass, VkDescriptorSetLayout dsl,
            const std::string &vertSPV, const std::string &fragSPV);

  void cleanup(VkDevice device);
};

} // namespace engine::render
