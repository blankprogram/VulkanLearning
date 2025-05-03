#include "engine/pipeline/GraphicsPipeline.hpp"

namespace engine {

vk::GraphicsPipelineCreateInfo GraphicsPipeline::makePipelineInfo(
    const PipelineLayout &layout, const vk::raii::RenderPass &renderPass,
    const vk::PipelineVertexInputStateCreateInfo &vertexInputInfo,
    const vk::PipelineInputAssemblyStateCreateInfo &inputAssembly,
    const vk::PipelineRasterizationStateCreateInfo &rasterizer,
    const vk::PipelineMultisampleStateCreateInfo &multisampling,
    const vk::PipelineDepthStencilStateCreateInfo &depthStencil,
    const vk::PipelineColorBlendStateCreateInfo &colorBlending,
    const std::vector<vk::PipelineShaderStageCreateInfo> &shaderStages,
    const std::vector<vk::PipelineViewportStateCreateInfo> &viewportStateInfos,
    const vk::PipelineDynamicStateCreateInfo &dynamicStateInfo,
    const Config &config) {
  vk::GraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.setStageCount(static_cast<uint32_t>(shaderStages.size()))
      .setPStages(shaderStages.data())
      .setPVertexInputState(&vertexInputInfo)
      .setPInputAssemblyState(&inputAssembly)
      .setPTessellationState(nullptr)
      .setPViewportState(viewportStateInfos.data())
      .setPRasterizationState(&rasterizer)
      .setPMultisampleState(&multisampling)
      .setPDepthStencilState(&depthStencil)
      .setPColorBlendState(&colorBlending)
      .setPDynamicState(&dynamicStateInfo)
      .setLayout(*layout.get())
      .setRenderPass(*renderPass)
      .setSubpass(0);
  return pipelineInfo;
}

GraphicsPipeline::GraphicsPipeline(
    const vk::raii::Device &device, const PipelineLayout &layout,
    const vk::raii::RenderPass &renderPass,
    const vk::PipelineVertexInputStateCreateInfo &vertexInputInfo,
    const vk::PipelineInputAssemblyStateCreateInfo &inputAssembly,
    const vk::PipelineRasterizationStateCreateInfo &rasterizer,
    const vk::PipelineMultisampleStateCreateInfo &multisampling,
    const vk::PipelineDepthStencilStateCreateInfo &depthStencil,
    const vk::PipelineColorBlendStateCreateInfo &colorBlending,
    const std::vector<vk::PipelineShaderStageCreateInfo> &shaderStages,
    const std::vector<vk::PipelineViewportStateCreateInfo> &viewportStateInfos,
    const vk::PipelineDynamicStateCreateInfo &dynamicStateInfo,
    const Config &config)
    : pipeline_{device, nullptr,
                makePipelineInfo(layout, renderPass, vertexInputInfo,
                                 inputAssembly, rasterizer, multisampling,
                                 depthStencil, colorBlending, shaderStages,
                                 viewportStateInfos, dynamicStateInfo,
                                 config)} {}

} // namespace engine
