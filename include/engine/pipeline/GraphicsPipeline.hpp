#pragma once

#include "engine/pipeline/PipelineLayout.hpp"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class GraphicsPipeline {
public:
  struct Config {
    vk::Extent2D viewportExtent;
    vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1;
  };

  GraphicsPipeline(
      const vk::raii::Device &device, const PipelineLayout &layout,
      const vk::raii::RenderPass &renderPass,
      const vk::PipelineVertexInputStateCreateInfo &vertexInputInfo,
      const vk::PipelineInputAssemblyStateCreateInfo &inputAssembly,
      const vk::PipelineRasterizationStateCreateInfo &rasterizer,
      const vk::PipelineMultisampleStateCreateInfo &multisampling,
      const vk::PipelineDepthStencilStateCreateInfo &depthStencil,
      const vk::PipelineColorBlendStateCreateInfo &colorBlending,
      const std::vector<vk::PipelineShaderStageCreateInfo> &shaderStages,
      const std::vector<vk::PipelineViewportStateCreateInfo>
          &viewportStateInfos,
      const vk::PipelineDynamicStateCreateInfo &dynamicStateInfo,
      const Config &config);

  ~GraphicsPipeline() = default;
  GraphicsPipeline(const GraphicsPipeline &) = delete;
  GraphicsPipeline &operator=(const GraphicsPipeline &) = delete;
  GraphicsPipeline(GraphicsPipeline &&) = default;
  GraphicsPipeline &operator=(GraphicsPipeline &&) = default;

  const vk::raii::Pipeline &get() const { return pipeline_; }

private:
  static vk::GraphicsPipelineCreateInfo makePipelineInfo(
      const PipelineLayout &layout, const vk::raii::RenderPass &renderPass,
      const vk::PipelineVertexInputStateCreateInfo &vertexInputInfo,
      const vk::PipelineInputAssemblyStateCreateInfo &inputAssembly,
      const vk::PipelineRasterizationStateCreateInfo &rasterizer,
      const vk::PipelineMultisampleStateCreateInfo &multisampling,
      const vk::PipelineDepthStencilStateCreateInfo &depthStencil,
      const vk::PipelineColorBlendStateCreateInfo &colorBlending,
      const std::vector<vk::PipelineShaderStageCreateInfo> &shaderStages,
      const std::vector<vk::PipelineViewportStateCreateInfo>
          &viewportStateInfos,
      const vk::PipelineDynamicStateCreateInfo &dynamicStateInfo,
      const Config &config);

  vk::raii::Pipeline pipeline_;
};

} // namespace engine
