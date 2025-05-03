#include "engine/rendering/Semaphore.hpp"

namespace engine {

static vk::raii::Semaphore makeSemaphore(const vk::raii::Device &device) {
  vk::SemaphoreCreateInfo ci{};
  return vk::raii::Semaphore{device, ci};
}

Semaphore::Semaphore(const vk::raii::Device &device)
    : sem_{makeSemaphore(device)} {}

} // namespace engine
