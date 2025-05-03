#pragma once

#include "engine/swapchain/Image.hpp"
#include "engine/swapchain/ImageView.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace engine::swapchain {

class DepthBuffer {
public:
  DepthBuffer(const vk::raii::PhysicalDevice &physical,
              const vk::raii::Device &device, vk::Extent2D extent);

  ~DepthBuffer() = default;
  DepthBuffer(const DepthBuffer &) = delete;
  DepthBuffer &operator=(const DepthBuffer &) = delete;
  DepthBuffer(DepthBuffer &&) = default;
  DepthBuffer &operator=(DepthBuffer &&) = default;

  const vk::raii::ImageView &getView() const { return view_.get(); }
  vk::Format getFormat() const { return format_; }
  vk::Extent2D getExtent() const { return extent_; }

private:
  static vk::Format
  findSupportedDepthFormat(const vk::raii::PhysicalDevice &physical);

  vk::Format format_;
  vk::Extent2D extent_;
  Image image_;
  ImageView view_;
};

} // namespace engine::swapchain
