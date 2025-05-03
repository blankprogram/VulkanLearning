#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace engine {

class Image {
public:
  struct Params {
    vk::Extent3D extent;
    vk::Format format;
    vk::ImageTiling tiling;
    vk::ImageUsageFlags usage;
    vk::MemoryPropertyFlags properties;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    vk::ImageCreateFlags flags = {};
  };

  Image(const vk::raii::PhysicalDevice &physical,
        const vk::raii::Device &device, const Params &params);

  ~Image() = default;
  Image(const Image &) = delete;
  Image &operator=(const Image &) = delete;
  Image(Image &&) = default;
  Image &operator=(Image &&) = default;

  vk::Image get() const { return *image_; }
  vk::Format format() const { return format_; }
  vk::Extent3D extent() const { return extent_; }
  vk::DeviceMemory memory() const { return *memory_; }

private:
  struct Bundle {
    vk::raii::Image image;
    vk::raii::DeviceMemory memory;
    vk::Format format;
    vk::Extent3D extent;
  };
  Image(Bundle &&b);

  static Bundle createBundle(const vk::raii::PhysicalDevice &physical,
                             const vk::raii::Device &device,
                             const Params &params);

  vk::raii::Image image_;
  vk::raii::DeviceMemory memory_;
  vk::Format format_;
  vk::Extent3D extent_;
};

} // namespace engine
