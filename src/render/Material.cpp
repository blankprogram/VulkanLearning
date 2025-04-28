#include "engine/render/Material.hpp"
#include <vulkan/vulkan.h>

Material::Material(VkDevice device, VkDescriptorPool descriptorPool,
                   VkDescriptorSetLayout layout, Texture *texture)
    : device_(device) {
  // Allocate a descriptor set for this material
  VkDescriptorSetAllocateInfo allocInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &layout;
  vkAllocateDescriptorSets(device_, &allocInfo, &descriptorSet_);

  // Write the texture+sampler into binding 0
  VkDescriptorImageInfo imgInfo{};
  imgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imgInfo.imageView = texture->GetImageView();
  imgInfo.sampler = texture->GetSampler();

  VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  write.dstSet = descriptorSet_;
  write.dstBinding = 0;
  write.dstArrayElement = 0;
  write.descriptorCount = 1;
  write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  write.pImageInfo = &imgInfo;

  vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);
}

Material::~Material() {
  // Optional: free descriptor set explicitly if you like
  // vkFreeDescriptorSets(device_, descriptorPool_, 1, &descriptorSet_);
}

void Material::Bind(VkCommandBuffer cmd, VkPipelineLayout pipelineLayout,
                    uint32_t setIndex) const {
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                          setIndex, 1, &descriptorSet_, 0, nullptr);
}
