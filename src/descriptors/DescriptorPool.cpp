#include "engine/descriptors/DescriptorPool.hpp"

namespace engine {

DescriptorPool::DescriptorPool(
    const vk::raii::Device &device,
    const std::vector<vk::DescriptorPoolSize> &poolSizes,
    uint32_t maxSets) noexcept
    : pool_{device,
            vk::DescriptorPoolCreateInfo{}
                .setPoolSizeCount(static_cast<uint32_t>(poolSizes.size()))
                .setPPoolSizes(poolSizes.data())
                .setMaxSets(maxSets)} {}

const vk::raii::DescriptorPool &DescriptorPool::get() const noexcept {
  return pool_;
}

} // namespace engine
