#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace engine {

class ImageView {
public:
  ImageView(const vk::raii::Device &device, vk::Image image, vk::Format format,
            vk::ImageAspectFlags aspectMask, uint32_t mipLevels = 1,
            uint32_t arrayLayers = 1);

  ~ImageView() = default;
  ImageView(const ImageView &) = delete;
  ImageView &operator=(const ImageView &) = delete;
  ImageView(ImageView &&) = default;
  ImageView &operator=(ImageView &&) = default;

  const vk::raii::ImageView &get() const { return view_; }

private:
  vk::raii::ImageView view_;
};

} // namespace engine
