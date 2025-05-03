
// src/swapchain/DepthBuffer.cpp
#include "engine/swapchain/Depthbuffer.hpp"
#include <stdexcept>

namespace engine::swapchain {

vk::Format DepthBuffer::findSupportedDepthFormat(
    const vk::raii::PhysicalDevice &physical) {
  std::array<vk::Format, 3> candidates = {vk::Format::eD32Sfloat,
                                          vk::Format::eD32SfloatS8Uint,
                                          vk::Format::eD24UnormS8Uint};

  for (auto fmt : candidates) {
    auto props = physical.getFormatProperties(fmt);
    if (props.optimalTilingFeatures &
        vk::FormatFeatureFlagBits::eDepthStencilAttachment)
      return fmt;
  }

  throw std::runtime_error("No supported depth format found");
}

DepthBuffer::DepthBuffer(const vk::raii::PhysicalDevice &physical,
                         const vk::raii::Device &device, vk::Extent2D extent)
    : format_(findSupportedDepthFormat(physical)), extent_(extent),
      image_{physical, device,
             Image::Params{vk::Extent3D{extent.width, extent.height, 1},
                           format_, vk::ImageTiling::eOptimal,
                           vk::ImageUsageFlagBits::eDepthStencilAttachment,
                           vk::MemoryPropertyFlagBits::eDeviceLocal}},
      view_{device, image_.get(), format_, vk::ImageAspectFlagBits::eDepth, 1,
            1} {}

} // namespace engine::swapchain
