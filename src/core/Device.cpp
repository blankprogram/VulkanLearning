#include "engine/core/Device.hpp"
#include "engine/configs/DeviceConfig.hpp"
#include <set>
#include <vector>
namespace engine {

namespace detail {

vk::raii::Device makeDevice(const vk::raii::PhysicalDevice &physical,
                            const Queue::FamilyIndices &indices) {
  std::set<uint32_t> families = {
      indices.graphics.value(), indices.present.value(),
      indices.compute.value(), indices.transfer.value()};

  float priority = 1.0f;
  std::vector<vk::DeviceQueueCreateInfo> queueInfos;
  queueInfos.reserve(families.size());
  for (auto family : families) {
    queueInfos.emplace_back(vk::DeviceQueueCreateInfo{}
                                .setQueueFamilyIndex(family)
                                .setQueueCount(1)
                                .setPQueuePriorities(&priority));
  }

  vk::PhysicalDeviceFeatures features{};
  if (deviceConfig.enableAnisotropy)
    features.setSamplerAnisotropy(true);
  if (deviceConfig.enableWideLines)
    features.setWideLines(true);

  std::vector<const char *> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  vk::DeviceCreateInfo info{};
  info.setQueueCreateInfos(queueInfos)
      .setPEnabledFeatures(&features)
      .setEnabledExtensionCount(static_cast<uint32_t>(extensions.size()))
      .setPpEnabledExtensionNames(extensions.data());

  return vk::raii::Device{physical, info};
}

} // namespace detail

Device::Device(const vk::raii::PhysicalDevice &physical,
               const vk::raii::SurfaceKHR &surface)
    : indices_(Queue::findQueueFamilies(physical, surface)),
      device_(detail::makeDevice(physical, indices_)),
      graphicsQueue_(device_, indices_.graphics.value(), 0),
      presentQueue_(device_, indices_.present.value(), 0),
      computeQueue_(device_, indices_.compute.value(), 0),
      transferQueue_(device_, indices_.transfer.value(), 0) {}

} // namespace engine
