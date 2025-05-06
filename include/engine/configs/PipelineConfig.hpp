
#pragma once

#include "engine/utils/Vertex.hpp"
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

struct PipelineConfig {
  // ---- you already had these ----
  std::vector<vk::DescriptorSetLayout> setLayouts;
  std::vector<vk::PushConstantRange> pushConstants;
  std::vector<vk::DynamicState> dynamicStates;
  std::vector<vk::PipelineColorBlendAttachmentState> blendAttachments;

  // —— NEW: store these so their pointers stay valid ——
  vk::VertexInputBindingDescription bindingDescription;
  std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;

  vk::PipelineVertexInputStateCreateInfo vertexInput{};
  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
  vk::PipelineViewportStateCreateInfo viewportState{};
  vk::PipelineRasterizationStateCreateInfo rasterizer{};
  vk::PipelineMultisampleStateCreateInfo multisampling{};
  vk::PipelineDepthStencilStateCreateInfo depthStencil{};
  vk::PipelineColorBlendStateCreateInfo colorBlend{};
  vk::PipelineDynamicStateCreateInfo dynamicState{};

  vk::Viewport viewport;
  vk::Rect2D scissor;
  vk::Extent2D viewportExtent;
  vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
};

inline PipelineConfig defaultPipelineConfig(vk::Extent2D extent) {
  PipelineConfig c;

  // — viewport / scissor —
  c.viewportExtent = extent;
  c.viewport =
      vk::Viewport{0, 0, float(extent.width), float(extent.height), 0.f, 1.f};
  c.scissor = vk::Rect2D{{0, 0}, extent};
  c.viewportState.setViewportCount(1)
      .setPViewports(&c.viewport)
      .setScissorCount(1)
      .setPScissors(&c.scissor);

  // — vertex input —
  // pull binding/attribute descriptions *into* the struct
  c.bindingDescription = Vertex::getBindingDescription();
  auto attrs = Vertex::getAttributeDescriptions();
  c.attributeDescriptions.assign(attrs.begin(), attrs.end());

  c.vertexInput.setVertexBindingDescriptionCount(1)
      .setPVertexBindingDescriptions(&c.bindingDescription)
      .setVertexAttributeDescriptionCount(
          static_cast<uint32_t>(c.attributeDescriptions.size()))
      .setPVertexAttributeDescriptions(c.attributeDescriptions.data());

  // — assembly —
  c.inputAssembly.setTopology(vk::PrimitiveTopology::eTriangleList)
      .setPrimitiveRestartEnable(VK_FALSE);

  // — rasterizer —
  c.rasterizer.setDepthClampEnable(VK_FALSE)
      .setRasterizerDiscardEnable(VK_FALSE)
      .setPolygonMode(vk::PolygonMode::eFill)
      .setLineWidth(1.f)
      .setCullMode(vk::CullModeFlagBits::eNone)
      .setFrontFace(vk::FrontFace::eClockwise)
      .setDepthBiasEnable(VK_FALSE);

  // — multisampling —
  c.multisampling.setRasterizationSamples(c.msaaSamples)
      .setSampleShadingEnable(VK_FALSE);

  // — depth/stencil —
  c.depthStencil.setDepthTestEnable(VK_TRUE)
      .setDepthWriteEnable(VK_TRUE)
      .setDepthCompareOp(vk::CompareOp::eLess)
      .setDepthBoundsTestEnable(VK_FALSE)
      .setStencilTestEnable(VK_FALSE);

  // — color blend —
  vk::PipelineColorBlendAttachmentState blendAtt{};
  blendAtt.setBlendEnable(VK_FALSE).setColorWriteMask(
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
  c.blendAttachments = {blendAtt};
  c.colorBlend.setLogicOpEnable(VK_FALSE)
      .setAttachmentCount(static_cast<uint32_t>(c.blendAttachments.size()))
      .setPAttachments(c.blendAttachments.data());

  // — dynamic state —
  c.dynamicStates = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
  c.dynamicState
      .setDynamicStateCount(static_cast<uint32_t>(c.dynamicStates.size()))
      .setPDynamicStates(c.dynamicStates.data());

  return c;
}

} // namespace engine
