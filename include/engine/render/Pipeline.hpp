
#pragma once

#include <string>
#include <vulkan/vulkan.h>

namespace engine::render {

struct Pipeline {
    VkPipeline pipeline{VK_NULL_HANDLE};
    VkPipelineLayout layout{VK_NULL_HANDLE};

    void init(VkDevice device, VkRenderPass renderPass,
              VkDescriptorSetLayout dsl, const std::string &vertSPV,
              const std::string &fragSPV);

    void cleanup(VkDevice device);
};

} // namespace engine::render
