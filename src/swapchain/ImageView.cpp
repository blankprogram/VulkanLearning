#include "engine/swapchain/ImageView.hpp"

namespace engine {

ImageView::ImageView(const vk::raii::Device &device, vk::Image image,
                     vk::Format format, vk::ImageAspectFlags aspectMask,
                     uint32_t mipLevels, uint32_t arrayLayers)
    : view_{device,
            vk::ImageViewCreateInfo{}
                .setImage(image)
                .setViewType(arrayLayers > 1 ? vk::ImageViewType::e2DArray
                                             : vk::ImageViewType::e2D)
                .setFormat(format)
                .setSubresourceRange(vk::ImageSubresourceRange{}
                                         .setAspectMask(aspectMask)
                                         .setBaseMipLevel(0)
                                         .setLevelCount(mipLevels)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(arrayLayers))} {}

} // namespace engine
