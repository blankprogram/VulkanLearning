
#include "engine/render/Pipeline.hpp"
#include <fstream>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

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
                    VkDescriptorSetLayout globalLayout,
                    VkDescriptorSetLayout materialLayout,
                    const std::string &vertPath, const std::string &fragPath,
                    VkPolygonMode polygonMode) {
  auto vertCode = readFile(vertPath);
  auto fragCode = readFile(fragPath);
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

  // 2) Vertex input
  auto bindingDesc = Vertex::getBindingDesc();
  auto attribDescs = Vertex::getAttributeDescs();
  VkPipelineVertexInputStateCreateInfo viCI{
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
  viCI.vertexBindingDescriptionCount = 1;
  viCI.pVertexBindingDescriptions = &bindingDesc;
  viCI.vertexAttributeDescriptionCount = uint32_t(attribDescs.size());
  viCI.pVertexAttributeDescriptions = attribDescs.data();

  // 3) Input assembly
  VkPipelineInputAssemblyStateCreateInfo iaCI{
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
  iaCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  iaCI.primitiveRestartEnable = VK_FALSE;

  // 4) Viewport + scissor (dynamic)
  VkViewport viewport{0.0f, 0.0f, float(extent.width), float(extent.height),
                      0.0f, 1.0f};
  VkRect2D scissor{{0, 0}, extent};
  std::vector<VkDynamicState> dynStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                           VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynCI{
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
  dynCI.dynamicStateCount = uint32_t(dynStates.size());
  dynCI.pDynamicStates = dynStates.data();

  VkPipelineViewportStateCreateInfo vpCI{
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
  vpCI.viewportCount = 1;
  vpCI.scissorCount = 1;

  // 5) Rasterizer
  VkPipelineRasterizationStateCreateInfo rsCI{
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
  rsCI.depthClampEnable = VK_FALSE;
  rsCI.rasterizerDiscardEnable = VK_FALSE;
  rsCI.polygonMode = polygonMode;
  rsCI.lineWidth = 1.0f;
  rsCI.cullMode = VK_CULL_MODE_NONE;
  rsCI.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rsCI.depthBiasEnable = VK_FALSE;

  // 6) Multisampling
  VkPipelineMultisampleStateCreateInfo msCI{
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
  msCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  // 7) Depth + stencil
  VkPipelineDepthStencilStateCreateInfo dsCI{
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
  dsCI.depthTestEnable = VK_TRUE;
  dsCI.depthWriteEnable = VK_TRUE;
  dsCI.depthCompareOp = VK_COMPARE_OP_LESS;
  dsCI.depthBoundsTestEnable = VK_FALSE;
  dsCI.stencilTestEnable = VK_FALSE;

  // 8) Color blending
  VkPipelineColorBlendAttachmentState cbAttach{};
  cbAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                            VK_COLOR_COMPONENT_G_BIT |
                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  cbAttach.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo cbCI{
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  cbCI.attachmentCount = 1;
  cbCI.pAttachments = &cbAttach;

  // 9) Pipeline layout: use TWO set layouts
  VkPushConstantRange pcRange{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)};
  VkDescriptorSetLayout setLayouts[] = {globalLayout, materialLayout};
  VkPipelineLayoutCreateInfo layoutCI{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  layoutCI.setLayoutCount = 2;
  layoutCI.pSetLayouts = setLayouts;
  layoutCI.pushConstantRangeCount = 1;
  layoutCI.pPushConstantRanges = &pcRange;

  if (vkCreatePipelineLayout(device, &layoutCI, nullptr, &pipelineLayout_) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to create pipeline layout");

  // 10) Graphics pipeline
  VkGraphicsPipelineCreateInfo gpCI{
      VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
  gpCI.stageCount = 2;
  gpCI.pStages = stages;
  gpCI.pVertexInputState = &viCI;
  gpCI.pInputAssemblyState = &iaCI;
  gpCI.pViewportState = &vpCI;
  gpCI.pRasterizationState = &rsCI;
  gpCI.pMultisampleState = &msCI;
  gpCI.pDepthStencilState = &dsCI;
  gpCI.pColorBlendState = &cbCI;
  gpCI.pDynamicState = &dynCI;
  gpCI.layout = pipelineLayout_;
  gpCI.renderPass = renderPass;
  gpCI.subpass = 0;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &gpCI, nullptr,
                                &graphicsPipeline_) != VK_SUCCESS)
    throw std::runtime_error("Failed to create graphics pipeline");

  // cleanup...
  vkDestroyShaderModule(device, fragShader, nullptr);
  vkDestroyShaderModule(device, vertShader, nullptr);
}
void Pipeline::Cleanup(VkDevice device) {
  vkDestroyPipeline(device, graphicsPipeline_, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
}
