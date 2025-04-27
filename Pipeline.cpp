
#include "Pipeline.h"
#include <fstream>
#include <stdexcept>

std::vector<char> Pipeline::readFile(const std::string &path) {
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  if (!file)
    throw std::runtime_error("Failed to open shader file: " + path);

  size_t size = (size_t)file.tellg();
  std::vector<char> buffer(size);
  file.seekg(0);
  file.read(buffer.data(), size);
  return buffer;
}

VkShaderModule Pipeline::createShaderModule(VkDevice device,
                                            const std::vector<char> &code) {
  VkShaderModuleCreateInfo ci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  ci.codeSize = code.size();
  ci.pCode = reinterpret_cast<const uint32_t *>(code.data());
  VkShaderModule module;
  if (vkCreateShaderModule(device, &ci, nullptr, &module) != VK_SUCCESS)
    throw std::runtime_error("Failed to create shader module");
  return module;
}

void Pipeline::Init(VkDevice device, VkExtent2D extent, VkRenderPass renderPass,
                    VkDescriptorSetLayout descriptorSetLayout) {
  // 1) load SPIR-V binaries
  auto vertCode = readFile("shaders/triangle.vert.spv");
  auto fragCode = readFile("shaders/triangle.frag.spv");

  VkShaderModule vertShader = createShaderModule(device, vertCode);
  VkShaderModule fragShader = createShaderModule(device, fragCode);

  VkPipelineShaderStageCreateInfo stages[2]{};
  stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = vertShader;
  stages[0].pName = "main";
  stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = fragShader;
  stages[1].pName = "main";

  // 2) vertex input (binding + attributes from your Vertex struct)
  auto bindingDesc = Vertex::getBindingDesc();
  auto attribDescs = Vertex::getAttributeDescs();
  VkPipelineVertexInputStateCreateInfo vertexInputCI{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  vertexInputCI.vertexBindingDescriptionCount = 1;
  vertexInputCI.pVertexBindingDescriptions = &bindingDesc;
  vertexInputCI.vertexAttributeDescriptionCount = uint32_t(attribDescs.size());
  vertexInputCI.pVertexAttributeDescriptions = attribDescs.data();

  // 3) input assembly
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI{
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyCI.primitiveRestartEnable = VK_FALSE;

  // 4) viewport & scissor
  VkViewport viewport{};
  viewport.x = 0;
  viewport.y = 0;
  viewport.width = float(extent.width);
  viewport.height = float(extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = extent;

  VkPipelineViewportStateCreateInfo vpCI{
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  vpCI.viewportCount = 1;
  vpCI.pViewports = &viewport;
  vpCI.scissorCount = 1;
  vpCI.pScissors = &scissor;

  // 5) rasterizer
  VkPipelineRasterizationStateCreateInfo rasterCI{
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rasterCI.depthClampEnable = VK_FALSE;
  rasterCI.rasterizerDiscardEnable = VK_FALSE;
  rasterCI.polygonMode = VK_POLYGON_MODE_FILL;
  rasterCI.lineWidth = 1.0f;
  rasterCI.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterCI.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterCI.depthBiasEnable = VK_FALSE;

  // 6) multisampling
  VkPipelineMultisampleStateCreateInfo msCI{
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  msCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  // 7) color blending
  VkPipelineColorBlendAttachmentState blendAttach{};
  blendAttach.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  blendAttach.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo blendCI{
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  blendCI.attachmentCount = 1;
  blendCI.pAttachments = &blendAttach;

  // 8) pipeline layout (with your descriptor set layout)
  VkPipelineLayoutCreateInfo layoutCI{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  layoutCI.setLayoutCount = 1;
  layoutCI.pSetLayouts = &descriptorSetLayout;

  if (vkCreatePipelineLayout(device, &layoutCI, nullptr, &pipelineLayout_) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to create pipeline layout");

  // 9) finally create the graphics pipeline
  VkGraphicsPipelineCreateInfo gpCI{
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  gpCI.stageCount = 2;
  gpCI.pStages = stages;
  gpCI.pVertexInputState = &vertexInputCI;
  gpCI.pInputAssemblyState = &inputAssemblyCI;
  gpCI.pViewportState = &vpCI;
  gpCI.pRasterizationState = &rasterCI;
  gpCI.pMultisampleState = &msCI;
  gpCI.pColorBlendState = &blendCI;
  gpCI.layout = pipelineLayout_;
  gpCI.renderPass = renderPass;
  gpCI.subpass = 0;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gpCI, nullptr,
                                &graphicsPipeline_) != VK_SUCCESS)
    throw std::runtime_error("Failed to create graphics pipeline");

  // cleanup
  vkDestroyShaderModule(device, fragShader, nullptr);
  vkDestroyShaderModule(device, vertShader, nullptr);
}

void Pipeline::Cleanup(VkDevice device) {
  vkDestroyPipeline(device, graphicsPipeline_, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
}
