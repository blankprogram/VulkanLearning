#include "engine/descriptors/DescriptorSetLayout.hpp"

namespace engine {

DescriptorSetLayout::DescriptorSetLayout(
    const vk::raii::Device &device,
    const std::vector<vk::DescriptorSetLayoutBinding> &bindings)
    : layout_{device,
              vk::DescriptorSetLayoutCreateInfo{}
                  .setBindingCount(static_cast<uint32_t>(bindings.size()))
                  .setPBindings(bindings.data())} {}

const vk::raii::DescriptorSetLayout &DescriptorSetLayout::get() const noexcept {
  return layout_;
}

} // namespace engine
