#pragma once
#include "engine/resources/Texture.hpp"
#include <vulkan/vulkan.h>

/// A minimal material: holds a descriptor set that binds one texture.
/// You can extend this with multiple textures, push-constants, tints, etc.
class Material {
public:
  /// Allocate and write a descriptor-set for a single Texture
  Material(VkDevice device, VkDescriptorPool descriptorPool,
           VkDescriptorSetLayout layout, Texture *texture);

  ~Material();

  /// Bind this material's descriptor set at the given index
  void Bind(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
            uint32_t setIndex = 0) const;

private:
  VkDevice device_;
  VkDescriptorSet descriptorSet_;
  Texture *texture_;
};
