#pragma once

#include "engine/utils/Vertex.hpp"
#include <array>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

struct PipelineConfig {
  std::vector<vk::DescriptorSetLayout> setLayouts;
  std::vector<vk::PushConstantRange> pushConstants;
  vk::PipelineVertexInputStateCreateInfo vertexInput{};
  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
  vk::PipelineRasterizationStateCreateInfo rasterizer{};
  vk::PipelineMultisampleStateCreateInfo multisampling{};
  vk::PipelineDepthStencilStateCreateInfo depthStencil{};
  vk::PipelineColorBlendStateCreateInfo colorBlend{};
  std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
  vk::Viewport viewport{};
  vk::Rect2D scissor{};
  vk::PipelineViewportStateCreateInfo viewportState{};
  vk::PipelineDynamicStateCreateInfo dynamicState{};
  vk::Extent2D viewportExtent{};
  vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
};
inline PipelineConfig defaultPipelineConfig(vk::Extent2D extent) {
  PipelineConfig c;
  c.viewportExtent = extent;
  vk::VertexInputBindingDescription binding{};
  binding.binding = 0;
  binding.stride = sizeof(Vertex);
  binding.inputRate = vk::VertexInputRate::eVertex;

  std::array<vk::VertexInputAttributeDescription, 2> attrs;
  attrs[0].binding = 0;
  attrs[0].location = 0;
  attrs[0].format = vk::Format::eR32G32B32Sfloat;
  attrs[0].offset = offsetof(Vertex, pos);

  attrs[1].binding = 0;
  attrs[1].location = 1;
  attrs[1].format = vk::Format::eR32G32Sfloat;
  attrs[1].offset = offsetof(Vertex, uv);

  c.vertexInput.setVertexBindingDescriptionCount(1)
      .setPVertexBindingDescriptions(&binding)
      .setVertexAttributeDescriptionCount(static_cast<uint32_t>(attrs.size()))
      .setPVertexAttributeDescriptions(attrs.data());

  c.inputAssembly.setTopology(vk::PrimitiveTopology::eTriangleList)
      .setPrimitiveRestartEnable(VK_FALSE);

  c.rasterizer.setDepthClampEnable(VK_FALSE)
      .setRasterizerDiscardEnable(VK_FALSE)
      .setPolygonMode(vk::PolygonMode::eFill)
      .setLineWidth(1.0f)
      .setCullMode(vk::CullModeFlagBits::eBack)
      .setFrontFace(vk::FrontFace::eClockwise)
      .setDepthBiasEnable(VK_FALSE);

  c.multisampling.setRasterizationSamples(c.msaaSamples)
      .setSampleShadingEnable(VK_FALSE);

  c.depthStencil.setDepthTestEnable(VK_TRUE)
      .setDepthWriteEnable(VK_TRUE)
      .setDepthCompareOp(vk::CompareOp::eLess)
      .setDepthBoundsTestEnable(VK_FALSE)
      .setStencilTestEnable(VK_FALSE);

  vk::PipelineColorBlendAttachmentState blendAtt{};
  blendAtt
      .setColorWriteMask(
          vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
          vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
      .setBlendEnable(VK_FALSE);
  c.colorBlend.setLogicOpEnable(VK_FALSE).setAttachmentCount(1).setPAttachments(
      &blendAtt);

  c.viewport = vk::Viewport{0.0f,
                            0.0f,
                            static_cast<float>(extent.width),
                            static_cast<float>(extent.height),
                            0.0f,
                            1.0f};
  c.scissor = vk::Rect2D{{0, 0}, extent};
  c.viewportState.setViewportCount(1)
      .setPViewports(&c.viewport)
      .setScissorCount(1)
      .setPScissors(&c.scissor);

  return c;
}
inline const PipelineConfig pipelineConfig{};
} // namespace engine
