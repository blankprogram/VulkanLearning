#include "engine/swapchain/Swapchain.hpp"
#include <algorithm> // std::clamp

namespace engine {

Swapchain::Swapchain(const vk::raii::PhysicalDevice &physical,
                     const vk::raii::Device &device,
                     const vk::raii::SurfaceKHR &surface,
                     const vk::Extent2D &windowExtent,
                     const Queue::FamilyIndices &indices,
                     VkSwapchainKHR oldSwapchain)
    : Swapchain(createBundle(physical, device, surface, windowExtent, indices,
                             oldSwapchain)) {}

Swapchain::Swapchain(Bundle &&bundle)
    : swapchain_(std::move(bundle.swapchain)), images_(swapchain_.getImages()),
      format_(bundle.format), extent_(bundle.extent) {}

Swapchain::Bundle Swapchain::createBundle(
    const vk::raii::PhysicalDevice &physical, const vk::raii::Device &device,
    const vk::raii::SurfaceKHR &surface, const vk::Extent2D &windowExtent,
    const Queue::FamilyIndices &indices, VkSwapchainKHR oldSwapchain) {
  auto caps = physical.getSurfaceCapabilitiesKHR(*surface);
  auto formats = physical.getSurfaceFormatsKHR(*surface);
  auto modes = physical.getSurfacePresentModesKHR(*surface);

  // 2) choose format
  vk::SurfaceFormatKHR chosenFmt = formats[0];
  for (auto &f : formats) {
    if (f.format == vk::Format::eB8G8R8A8Srgb &&
        f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      chosenFmt = f;
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

  vk::SwapchainCreateInfoKHR sci{};
  sci.surface = *surface;
  sci.minImageCount = imageCount;
  sci.imageFormat = chosenFmt.format;
  sci.imageColorSpace = chosenFmt.colorSpace;
  sci.imageExtent = actualExtent;
  sci.imageArrayLayers = 1;
  sci.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
  sci.preTransform = caps.currentTransform;
  sci.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  sci.presentMode = presentMode;
  sci.clipped = VK_TRUE;
  sci.oldSwapchain = oldSwapchain;

  uint32_t qfams[] = {indices.graphics.value(), indices.present.value()};
  if (indices.graphics != indices.present) {
    sci.imageSharingMode = vk::SharingMode::eConcurrent;
    sci.queueFamilyIndexCount = 2;
    sci.pQueueFamilyIndices = qfams;
  } else {
    sci.imageSharingMode = vk::SharingMode::eExclusive;
  }

  vk::raii::SwapchainKHR sc(device, sci);
  return Bundle{std::move(sc), chosenFmt.format, actualExtent};
}

} // namespace engine
