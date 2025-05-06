
#include "engine/swapchain/Swapchain.hpp"
#include <algorithm> // for std::clamp

namespace engine {

Swapchain::Swapchain(const vk::raii::PhysicalDevice &physical,
                     const vk::raii::Device &device,
                     const vk::raii::SurfaceKHR &surface,
                     const vk::Extent2D &windowExtent,
                     const Queue::FamilyIndices &indices)
    : Swapchain(
          createBundle(physical, device, surface, windowExtent, indices)) {}

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
  // 1) Query surface capabilities, formats, present modes
  auto caps = physical.getSurfaceCapabilitiesKHR(*surface);
  auto formats = physical.getSurfaceFormatsKHR(*surface);
  auto modes = physical.getSurfacePresentModesKHR(*surface);

  // 2) Pick preferred format
  vk::SurfaceFormatKHR surfaceFmt = formats[0];
  for (auto &f : formats) {
    if (f.format == vk::Format::eB8G8R8A8Srgb &&
        f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      surfaceFmt = f;
      break;
    }
  }

  // 3) Pick present mode
  vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
  for (auto &m : modes) {
    if (m == vk::PresentModeKHR::eMailbox) {
      presentMode = m;
      break;
    }
  }

  // 4) Determine swap extent
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

  // 5) Determine image count
  uint32_t imageCount = caps.minImageCount + 1;
  if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
    imageCount = caps.maxImageCount;

  // 6) Fill out CreateInfo
  vk::SwapchainCreateInfoKHR info{};
  info.surface = *surface;
  info.minImageCount = imageCount;
  info.imageFormat = surfaceFmt.format;
  info.imageColorSpace = surfaceFmt.colorSpace;
  info.imageExtent = actualExtent;
  info.imageArrayLayers = 1;
  info.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

  // 7) Handle concurrent vs exclusive sharing
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

  // <-- THIS LINE IS CRUCIAL TO avoid ErrorNativeWindowInUseKHR -->
  info.oldSwapchain = oldSwapchain;

  // 8) Create the RAII swapchain
  vk::raii::SwapchainKHR sc(device, info);

  return Bundle{std::move(sc), surfaceFmt.format, actualExtent};
}

} // namespace engine
