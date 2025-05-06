#include "engine/pipeline/RenderPass.hpp"
#include <array>

namespace engine {

// build-and-create helper: attachments, subpass & dependency live
// through the vk::raii::RenderPass constructor call
static vk::raii::RenderPass makeRenderPass(const vk::raii::Device &device,
                                           vk::Format colorFormat,
                                           vk::Format depthFormat) {
  // 0 = color, 1 = depth
  std::array<vk::AttachmentDescription, 2> atts = {
      // color
      vk::AttachmentDescription{}
          .setFormat(colorFormat)
          .setSamples(vk::SampleCountFlagBits::e1)
          .setLoadOp(vk::AttachmentLoadOp::eClear)
          .setStoreOp(vk::AttachmentStoreOp::eStore)
          .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
          .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
          .setInitialLayout(vk::ImageLayout::eUndefined)
          .setFinalLayout(vk::ImageLayout::ePresentSrcKHR),

      // depth
      vk::AttachmentDescription{}
          .setFormat(depthFormat)
          .setSamples(vk::SampleCountFlagBits::e1)
          .setLoadOp(vk::AttachmentLoadOp::eClear)
          .setStoreOp(vk::AttachmentStoreOp::eDontCare)
          .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
          .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
          .setInitialLayout(vk::ImageLayout::eUndefined)
          .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal),
  };

  // references into those attachments
  vk::AttachmentReference colorRef{0, vk::ImageLayout::eColorAttachmentOptimal};
  vk::AttachmentReference depthRef{
      1, vk::ImageLayout::eDepthStencilAttachmentOptimal};

  // single subpass uses both
  vk::SubpassDescription subpass{};
  subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
      .setColorAttachmentCount(1)
      .setPColorAttachments(&colorRef)
      .setPDepthStencilAttachment(&depthRef);

  // ensure proper transitions
  vk::SubpassDependency dep{};
  dep.setSrcSubpass(VK_SUBPASS_EXTERNAL)
      .setDstSubpass(0)
      .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                       vk::PipelineStageFlagBits::eEarlyFragmentTests)
      .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                       vk::PipelineStageFlagBits::eEarlyFragmentTests)
      .setSrcAccessMask({})
      .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite |
                        vk::AccessFlagBits::eDepthStencilAttachmentWrite);

  // assemble the create info
  vk::RenderPassCreateInfo info{};
  info.setAttachmentCount(static_cast<uint32_t>(atts.size()))
      .setPAttachments(atts.data())
      .setSubpassCount(1)
      .setPSubpasses(&subpass)
      .setDependencyCount(1)
      .setPDependencies(&dep);

  // call into RAII while all locals are alive
  return vk::raii::RenderPass{device, info};
}

RenderPass::RenderPass(const vk::raii::Device &device, vk::Format colorFormat,
                       vk::Format depthFormat)
    : renderPass_{makeRenderPass(device, colorFormat, depthFormat)} {}

} // namespace engine
