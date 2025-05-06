
#include "engine/pipeline/RenderPass.hpp"
#include <array>

namespace engine {

RenderPass::RenderPass(const vk::raii::Device &device, vk::Format colorFormat)
    : renderPass_{
          device,
          // 1 color attachment:
          vk::RenderPassCreateInfo{}
              .setAttachmentCount(1)
              .setPAttachments(std::array<vk::AttachmentDescription, 1>{
                  {vk::AttachmentDescription{}
                       .setFormat(colorFormat)
                       .setSamples(vk::SampleCountFlagBits::e1)
                       .setLoadOp(vk::AttachmentLoadOp::eClear)
                       .setStoreOp(vk::AttachmentStoreOp::eStore)
                       .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                       .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                       .setInitialLayout(vk::ImageLayout::eUndefined)
                       .setFinalLayout(vk::ImageLayout::ePresentSrcKHR)}}
                                   .data())

              // 1 subpass, using that attachment:
              .setSubpassCount(1)
              .setPSubpasses(std::array<vk::SubpassDescription, 1>{
                  {vk::SubpassDescription{}
                       .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                       .setColorAttachmentCount(1)
                       .setPColorAttachments(std::array<vk::AttachmentReference,
                                                        1>{
                           {vk::AttachmentReference{}
                                .setAttachment(0)
                                .setLayout(
                                    vk::ImageLayout::eColorAttachmentOptimal)}}
                                                 .data())}}
                                 .data())

              // 1 external → subpass dependency for the layout transition:
              .setDependencyCount(1)
              .setPDependencies(std::array<vk::SubpassDependency, 1>{
                  {vk::SubpassDependency{}
                       .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                       .setDstSubpass(0)
                       .setSrcStageMask(
                           vk::PipelineStageFlagBits::eColorAttachmentOutput)
                       .setDstStageMask(
                           vk::PipelineStageFlagBits::eColorAttachmentOutput)
                       .setSrcAccessMask({})
                       .setDstAccessMask(
                           vk::AccessFlagBits::eColorAttachmentWrite)}}
                                    .data())} {}

} // namespace engine
