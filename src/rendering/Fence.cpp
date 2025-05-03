#include "engine/rendering/Fence.hpp"

namespace engine {

static vk::raii::Fence makeFence(const vk::raii::Device &device,
                                 bool signaled) {
  vk::FenceCreateInfo fi{};
  if (signaled)
    fi.setFlags(vk::FenceCreateFlagBits::eSignaled);
  return vk::raii::Fence{device, fi};
}

Fence::Fence(const vk::raii::Device &device, bool signaled)
    : fence_{makeFence(device, signaled)}, device_{device} {}

void Fence::reset() { device_.resetFences({*fence_}); }

} // namespace engine
