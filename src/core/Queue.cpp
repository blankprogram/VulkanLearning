#include "engine/core/Queue.hpp"
#include <stdexcept>

namespace engine {

bool Queue::FamilyIndices::isComplete() const {
  return graphics.has_value() && present.has_value() && compute.has_value() &&
         transfer.has_value();
}

Queue::FamilyIndices
Queue::findQueueFamilies(const vk::raii::PhysicalDevice &physical,
                         const vk::raii::SurfaceKHR &surface) {
  FamilyIndices indices;
  auto queueFamilies = physical.getQueueFamilyProperties();

  for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
    const auto &props = queueFamilies[i];
    if (props.queueCount == 0)
      continue;

    if (props.queueFlags & vk::QueueFlagBits::eGraphics)
      indices.graphics = i;
    if (props.queueFlags & vk::QueueFlagBits::eCompute)
      indices.compute = i;
    if (props.queueFlags & vk::QueueFlagBits::eTransfer)
      indices.transfer = i;

    if (physical.getSurfaceSupportKHR(i, *surface))
      indices.present = i;

    if (indices.isComplete())
      break;
  }

  if (!indices.graphics.has_value() || !indices.present.has_value()) {
    throw std::runtime_error(
        "Required queue families (graphics + present) not found");
  }
  if (!indices.compute)
    indices.compute = indices.graphics;
  if (!indices.transfer)
    indices.transfer = indices.graphics;

  return indices;
}

} // namespace engine
