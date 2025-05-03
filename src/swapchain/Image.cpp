#include "engine/swapchain/Image.hpp"
#include <stdexcept>
#include <vulkan/vulkan.h>
namespace engine {

static uint32_t
findMemoryTypeIndex(const vk::PhysicalDeviceMemoryProperties &memProps,
                    uint32_t typeFilter, vk::MemoryPropertyFlags props) {
  for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
    if ((typeFilter & (1 << i)) &&
        (memProps.memoryTypes[i].propertyFlags & props) == props) {
      return i;
    }
  }
  throw std::runtime_error("No suitable memory type found");
}

Image::Bundle Image::createBundle(const vk::raii::PhysicalDevice &physical,
                                  const vk::raii::Device &device,
                                  const Params &params) {
  vk::ImageCreateInfo ci{};
  ci.setImageType(vk::ImageType::e2D)
      .setExtent(params.extent)
      .setMipLevels(params.mipLevels)
      .setArrayLayers(params.arrayLayers)
      .setFormat(params.format)
      .setTiling(params.tiling)
      .setUsage(params.usage)
      .setSharingMode(vk::SharingMode::eExclusive)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFlags(params.flags);
  vk::raii::Image image{device, ci};

  VkMemoryRequirements req;
  vkGetImageMemoryRequirements(static_cast<VkDevice>(*device),
                               static_cast<VkImage>(*image), &req);

  uint32_t memIdx = findMemoryTypeIndex(physical.getMemoryProperties(),
                                        req.memoryTypeBits, params.properties);
  vk::MemoryAllocateInfo ai{};
  ai.setAllocationSize(req.size).setMemoryTypeIndex(memIdx);
  vk::raii::DeviceMemory memory{device, ai};

  vkBindImageMemory(static_cast<VkDevice>(*device),
                    static_cast<VkImage>(*image),
                    static_cast<VkDeviceMemory>(*memory), 0);

  return Bundle{std::move(image), std::move(memory), params.format,
                params.extent};
}

Image::Image(const vk::raii::PhysicalDevice &p, const vk::raii::Device &d,
             const Params &pr)
    : Image(createBundle(p, d, pr)) {}

Image::Image(Bundle &&b)
    : image_(std::move(b.image)), memory_(std::move(b.memory)),
      format_(b.format), extent_(b.extent) {}

} // namespace engine
