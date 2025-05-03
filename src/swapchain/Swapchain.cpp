
#include "engine/swapchain/Swapchain.hpp"
#include <algorithm>
#include <set>

namespace engine {

Swapchain::Swapchain(const vk::raii::PhysicalDevice &physical,
                     const vk::raii::Device &device,
                     const vk::raii::SurfaceKHR &surface,
                     const vk::Extent2D &windowExtent,
                     const Queue::FamilyIndices &indices)
    : Swapchain(
          createBundle(physical, device, surface, windowExtent, indices)) {}

Swapchain::Swapchain(Bundle &&bundle)
    : swapchain_(std::move(bundle.swapchain)), images_(swapchain_.getImages()),
      format_(bundle.format), extent_(bundle.extent) {}

Swapchain::Bundle Swapchain::createBundle(
    const vk::raii::PhysicalDevice &physical, const vk::raii::Device &device,
    const vk::raii::SurfaceKHR &surface, const vk::Extent2D &windowExtent,
    const Queue::FamilyIndices &indices) {
  auto caps = physical.getSurfaceCapabilitiesKHR(*surface);
  auto formats = physical.getSurfaceFormatsKHR(*surface);
  auto modes = physical.getSurfacePresentModesKHR(*surface);

  vk::SurfaceFormatKHR surfaceFmt = formats[0];
  for (auto &f : formats) {
    if (f.format == vk::Format::eB8G8R8A8Srgb &&
        f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      surfaceFmt = f;
      break;
    }
  }

  vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
  for (auto &m : modes) {
    if (m == vk::PresentModeKHR::eMailbox) {
      presentMode = m;
      break;
    }
  }

  vk::Extent2D actualExtent;
  if (caps.currentExtent.width != UINT32_MAX) {
    actualExtent = caps.currentExtent;
  } else {
    actualExtent = windowExtent;
    actualExtent.width =
        std::clamp(actualExtent.width, caps.minImageExtent.width,
                   caps.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, caps.minImageExtent.height,
                   caps.maxImageExtent.height);
  }

  uint32_t imageCount = caps.minImageCount + 1;
  if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
    imageCount = caps.maxImageCount;

  vk::SwapchainCreateInfoKHR info{};
  info.surface = *surface;
  info.minImageCount = imageCount;
  info.imageFormat = surfaceFmt.format;
  info.imageColorSpace = surfaceFmt.colorSpace;
  info.imageExtent = actualExtent;
  info.imageArrayLayers = 1;
  info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

  uint32_t qFamilies[] = {indices.graphics.value(), indices.present.value()};
  if (indices.graphics != indices.present) {
    info.imageSharingMode = vk::SharingMode::eConcurrent;
    info.queueFamilyIndexCount = 2;
    info.pQueueFamilyIndices = qFamilies;
  } else {
    info.imageSharingMode = vk::SharingMode::eExclusive;
  }

  info.preTransform = caps.currentTransform;
  info.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  info.presentMode = presentMode;
  info.clipped = VK_TRUE;
  info.oldSwapchain = nullptr;

  vk::raii::SwapchainKHR swapchain(device, info);

  return Bundle{std::move(swapchain), surfaceFmt.format, actualExtent};
}

} // namespace engine
