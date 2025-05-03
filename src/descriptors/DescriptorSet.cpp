#include "engine/descriptors/DescriptorSet.hpp"

namespace engine {

DescriptorSet::DescriptorSet(
    const vk::raii::Device &device, const vk::raii::DescriptorPool &pool,
    const std::vector<vk::DescriptorSetLayout> &layouts) noexcept
    : sets_{device,
            vk::DescriptorSetAllocateInfo{}
                .setDescriptorPool(*pool)
                .setDescriptorSetCount(static_cast<uint32_t>(layouts.size()))
                .setPSetLayouts(layouts.data())} {}

const vk::raii::DescriptorSet &DescriptorSet::get() const noexcept {
  return sets_.front();
}

const vk::raii::DescriptorSets &DescriptorSet::getAll() const noexcept {
  return sets_;
}

} // namespace engine
