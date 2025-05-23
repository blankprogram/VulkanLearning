#include "engine/render/Pipeline.hpp"
#include "engine/render/Vertex.hpp"
#include <fstream>
#include <glm/mat4x4.hpp>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace engine::render {

static std::vector<char> loadSPV(const std::string &path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
        throw std::runtime_error{"Failed to open SPIR-V file: " + path};
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);
    std::vector<char> buf(size);
    file.read(buf.data(), size);
    return buf;
}

void Pipeline::init(VkDevice dev, VkRenderPass rp, VkDescriptorSetLayout dsl,
                    const std::string &vpath, const std::string &fpath) {
    auto vcode = loadSPV(vpath);
    auto fcode = loadSPV(fpath);

    VkShaderModuleCreateInfo smci{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    smci.codeSize = vcode.size();
    smci.pCode = reinterpret_cast<const uint32_t *>(vcode.data());
    VkShaderModule vs;
    if (vkCreateShaderModule(dev, &smci, nullptr, &vs) != VK_SUCCESS)
        throw std::runtime_error{"Failed to create vertex shader module"};

    smci.codeSize = fcode.size();
    smci.pCode = reinterpret_cast<const uint32_t *>(fcode.data());
    VkShaderModule fs;
    if (vkCreateShaderModule(dev, &smci, nullptr, &fs) != VK_SUCCESS) {
        vkDestroyShaderModule(dev, vs, nullptr);
        throw std::runtime_error{"Failed to create fragment shader module"};
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vs;
    stages[0].pName = "main";
    stages[1] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fs;
    stages[1].pName = "main";

    VkVertexInputBindingDescription bind{0, sizeof(Vertex),
                                         VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attr[4] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
        {3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)}};

    VkPipelineVertexInputStateCreateInfo vis{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vis.vertexBindingDescriptionCount = 1;
    vis.pVertexBindingDescriptions = &bind;
    vis.vertexAttributeDescriptionCount = 4;
    vis.pVertexAttributeDescriptions = attr;

    VkPipelineInputAssemblyStateCreateInfo ias{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    ias.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo vpState{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    vpState.viewportCount = 1;
    vpState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo ras{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    ras.polygonMode = VK_POLYGON_MODE_FILL;
    ras.lineWidth = 1.0f;
    ras.cullMode = VK_CULL_MODE_BACK_BIT;
    ras.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo ms{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo ds{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState cba{};
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo cb{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    cb.attachmentCount = 1;
    cb.pAttachments = &cba;

    std::vector<VkDynamicState> dyn = {VK_DYNAMIC_STATE_VIEWPORT,
                                       VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dsc{
        VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dsc.dynamicStateCount = static_cast<uint32_t>(dyn.size());
    dsc.pDynamicStates = dyn.data();

    VkPushConstantRange push{};
    push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push.offset = 0;
    push.size = sizeof(glm::mat4);

    VkPipelineLayoutCreateInfo pli{
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pli.setLayoutCount = 1;
    pli.pSetLayouts = &dsl;
    pli.pushConstantRangeCount = 1;
    pli.pPushConstantRanges = &push;

    if (vkCreatePipelineLayout(dev, &pli, nullptr, &layout) != VK_SUCCESS)
        throw std::runtime_error{"Failed to create pipeline layout"};

    VkGraphicsPipelineCreateInfo gpi{
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gpi.stageCount = 2;
    gpi.pStages = stages;
    gpi.pVertexInputState = &vis;
    gpi.pInputAssemblyState = &ias;
    gpi.pViewportState = &vpState;
    gpi.pRasterizationState = &ras;
    gpi.pMultisampleState = &ms;
    gpi.pDepthStencilState = &ds;
    gpi.pColorBlendState = &cb;
    gpi.pDynamicState = &dsc;
    gpi.layout = layout;
    gpi.renderPass = rp;
    gpi.subpass = 0;

    if (vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &gpi, nullptr,
                                  &pipeline) != VK_SUCCESS)
        throw std::runtime_error{"Failed to create graphics pipeline"};

    vkDestroyShaderModule(dev, vs, nullptr);
    vkDestroyShaderModule(dev, fs, nullptr);
}

void Pipeline::cleanup(VkDevice device) {
    if (pipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(device, pipeline, nullptr);
    if (layout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device, layout, nullptr);
}

} // namespace engine::render
