#include "engine/pipeline/RenderPass.hpp"

namespace engine {

static vk::RenderPassCreateInfo
makeColorRenderPassInfo(vk::Format colorFormat) {
  vk::AttachmentDescription colorAttachment(
      {},                               // flags
      colorFormat,                      // format
      vk::SampleCountFlagBits::e1,      // samples
      vk::AttachmentLoadOp::eClear,     // loadOp
      vk::AttachmentStoreOp::eStore,    // storeOp
      vk::AttachmentLoadOp::eDontCare,  // stencilLoadOp
      vk::AttachmentStoreOp::eDontCare, // stencilStoreOp
      vk::ImageLayout::eUndefined,      // initialLayout
      vk::ImageLayout::ePresentSrcKHR   // finalLayout
  );

  // 2) Attachment reference for the subpass
  vk::AttachmentReference colorAttachmentRef(
      0,                                       // attachment index
      vk::ImageLayout::eColorAttachmentOptimal // layout
  );

  // 3) Single subpass, no preserves, no depth/stencil
  vk::SubpassDescription subpass(
      {},                               // flags
      vk::PipelineBindPoint::eGraphics, // pipelineBindPoint
      0, nullptr,                       // inputAttachments
      1, &colorAttachmentRef,           // colorAttachments
      nullptr,                          // resolveAttachments
      nullptr,                          // depthStencilAttachment
      0, nullptr                        // preserveAttachments
  );

  // 4) A dependency to make sure the transition from USER → color is handled
  vk::SubpassDependency dependency(
      VK_SUBPASS_EXTERNAL,                               // srcSubpass
      0,                                                 // dstSubpass
      vk::PipelineStageFlagBits::eColorAttachmentOutput, // srcStageMask
      vk::PipelineStageFlagBits::eColorAttachmentOutput, // dstStageMask
      {},                                                // srcAccessMask
      vk::AccessFlagBits::eColorAttachmentWrite          // dstAccessMask
  );

  // 5) Gather it all up
  vk::RenderPassCreateInfo rpInfo({},                  // flags
                                  1, &colorAttachment, // attachments
                                  1, &subpass,         // subpasses
                                  1, &dependency       // dependencies
  );

  return rpInfo;
}

RenderPass::RenderPass(const vk::raii::Device &device, vk::Format colorFormat)
    : renderPass_{device, makeColorRenderPassInfo(colorFormat)} {}

} // namespace engine
