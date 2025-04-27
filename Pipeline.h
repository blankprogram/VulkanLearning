
#pragma once
#include "Vertex.h"
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

class Pipeline {
public:
  // Call once after you have device, swapchainExtent & renderPass
  void Init(VkDevice device, VkExtent2D swapchainExtent,
            VkRenderPass renderPass);

  // Call during cleanup
  void Cleanup(VkDevice device);

  // Accessors for binding & cleanup
  VkPipeline GetPipeline() const { return graphicsPipeline_; }
  VkPipelineLayout GetLayout() const { return pipelineLayout_; }

private:
  VkShaderModule createShaderModule(VkDevice device,
                                    const std::vector<char> &code);
  std::vector<char> readFile(const std::string &filename);

  VkPipeline graphicsPipeline_{VK_NULL_HANDLE};
  VkPipelineLayout pipelineLayout_{VK_NULL_HANDLE};
};
