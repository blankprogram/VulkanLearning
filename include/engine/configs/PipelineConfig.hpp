#pragma once

#include "engine/utils/Vertex.hpp"
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

struct PipelineConfig {
  std::vector<vk::DescriptorSetLayout> setLayouts;
  std::vector<vk::PushConstantRange> pushConstants;
  std::vector<vk::VertexInputBindingDescription> bindingDescriptions;
  std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;

  // Owning containers for dynamic state and blend attachments
  std::vector<vk::DynamicState> dynamicStates;
  std::vector<vk::PipelineColorBlendAttachmentState> blendAttachments;

  // Pipeline state create infos
  vk::PipelineVertexInputStateCreateInfo vertexInput{};
  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
  vk::PipelineViewportStateCreateInfo viewportState{};
  vk::PipelineRasterizationStateCreateInfo rasterizer{};
  vk::PipelineMultisampleStateCreateInfo multisampling{};
  vk::PipelineDepthStencilStateCreateInfo depthStencil{};
  vk::PipelineColorBlendStateCreateInfo colorBlend{};
  vk::PipelineDynamicStateCreateInfo dynamicState{};

  // Viewport and scissor
  vk::Viewport viewport;
  vk::Rect2D scissor;
  vk::Extent2D viewportExtent;

  // MSAA samples
  vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
};

inline PipelineConfig defaultPipelineConfig(vk::Extent2D extent) {
  PipelineConfig c;

  c.setLayouts = {};
  c.pushConstants = {};
  c.viewportExtent = extent;
  c.viewport = vk::Viewport{
      0.0f, 0.0f, float(extent.width), float(extent.height), 0.0f, 1.0f};
  c.scissor = vk::Rect2D{{0, 0}, extent};
  c.viewportState.setViewportCount(1)
      .setPViewports(&c.viewport)
      .setScissorCount(1)
      .setPScissors(&c.scissor);

  // Vertex input binding
  vk::VertexInputBindingDescription binding{};
  binding.setBinding(0)
      .setStride(sizeof(Vertex))
      .setInputRate(vk::VertexInputRate::eVertex);
  c.bindingDescriptions = {binding};

  // Vertex input attributes (position + uv)
  auto attrs = Vertex::getAttributeDescriptions();
  c.attributeDescriptions = {attrs[0], attrs[2]};

  c.vertexInput
      .setVertexBindingDescriptionCount(
          static_cast<uint32_t>(c.bindingDescriptions.size()))
      .setPVertexBindingDescriptions(c.bindingDescriptions.data())
      .setVertexAttributeDescriptionCount(
          static_cast<uint32_t>(c.attributeDescriptions.size()))
      .setPVertexAttributeDescriptions(c.attributeDescriptions.data());

  // Input assembly
  c.inputAssembly.setTopology(vk::PrimitiveTopology::eTriangleList)
      .setPrimitiveRestartEnable(VK_FALSE);

  // Rasterizer
  c.rasterizer.setDepthClampEnable(VK_FALSE)
      .setRasterizerDiscardEnable(VK_FALSE)
      .setPolygonMode(vk::PolygonMode::eFill)
      .setLineWidth(1.0f)
      .setCullMode(vk::CullModeFlagBits::eBack)
      .setFrontFace(vk::FrontFace::eClockwise)
      .setDepthBiasEnable(VK_FALSE);

  // Multisampling
  c.multisampling.setRasterizationSamples(c.msaaSamples)
      .setSampleShadingEnable(VK_FALSE);

  // Depth / stencil
  c.depthStencil.setDepthTestEnable(VK_TRUE)
      .setDepthWriteEnable(VK_TRUE)
      .setDepthCompareOp(vk::CompareOp::eLess)
      .setDepthBoundsTestEnable(VK_FALSE)
      .setStencilTestEnable(VK_FALSE);

  // Color blend attachment (no blending)
  vk::PipelineColorBlendAttachmentState blendAtt{};
  blendAtt.setBlendEnable(VK_FALSE)
      .setColorWriteMask(
          vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setSrcColorBlendFactor(vk::BlendFactor::eOne)
      .setDstColorBlendFactor(vk::BlendFactor::eZero)
      .setColorBlendOp(vk::BlendOp::eAdd)
      .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
      .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
      .setAlphaBlendOp(vk::BlendOp::eAdd);
  c.blendAttachments = {blendAtt};

  c.colorBlend.setLogicOpEnable(VK_FALSE)
      .setAttachmentCount(static_cast<uint32_t>(c.blendAttachments.size()))
      .setPAttachments(c.blendAttachments.data());

  // Dynamic states (viewport + scissor)
  c.dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  c.dynamicState
      .setDynamicStateCount(static_cast<uint32_t>(c.dynamicStates.size()))
      .setPDynamicStates(c.dynamicStates.data());

  return c;
}

inline const PipelineConfig pipelineConfig{};

} // namespace engine
