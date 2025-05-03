#include "engine/swapchain/Framebuffer.hpp"

namespace engine::swapchain {

static vk::FramebufferCreateInfo
makeFramebufferInfo(const vk::raii::RenderPass &renderPass, vk::Extent2D extent,
                    const std::vector<vk::ImageView> &attachments) {
  vk::FramebufferCreateInfo info{};
  info.setRenderPass(*renderPass)
      .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
      .setPAttachments(attachments.data())
      .setWidth(extent.width)
      .setHeight(extent.height)
      .setLayers(1);
  return info;
}

Framebuffer::Framebuffer(const vk::raii::Device &device,
                         const vk::raii::RenderPass &renderPass,
                         vk::Extent2D extent,
                         const std::vector<vk::ImageView> &attachments)
    : framebuffer_{device,
                   makeFramebufferInfo(renderPass, extent, attachments)} {}

} // namespace engine::swapchain
