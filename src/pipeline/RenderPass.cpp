#include "engine/pipeline/RenderPass.hpp"

namespace engine {

static vk::RenderPassCreateInfo
makeColorRenderPassInfo(vk::Format colorFormat) {
  vk::AttachmentDescription colorAttachment{};
  colorAttachment.setFormat(colorFormat)
      .setSamples(vk::SampleCountFlagBits::e1)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

  vk::AttachmentReference colorRef{};
  colorRef.setAttachment(0).setLayout(vk::ImageLayout::eColorAttachmentOptimal);

  vk::SubpassDescription subpass{};
  subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
      .setColorAttachmentCount(1)
      .setPColorAttachments(&colorRef);

  vk::SubpassDependency dependency{};
  dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
      .setDstSubpass(0)
      .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setSrcAccessMask({})
      .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

  vk::RenderPassCreateInfo rpInfo{};
  rpInfo.setAttachmentCount(1)
      .setPAttachments(&colorAttachment)
      .setSubpassCount(1)
      .setPSubpasses(&subpass)
      .setDependencyCount(1)
      .setPDependencies(&dependency);

  return rpInfo;
}

RenderPass::RenderPass(const vk::raii::Device &device, vk::Format colorFormat)
    : renderPass_{device, makeColorRenderPassInfo(colorFormat)} {}

} // namespace engine
