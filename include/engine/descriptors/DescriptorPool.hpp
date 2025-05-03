#pragma once

#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class DescriptorPool {
public:
  DescriptorPool(const vk::raii::Device &device,
                 const std::vector<vk::DescriptorPoolSize> &poolSizes,
                 uint32_t maxSets) noexcept;

  const vk::raii::DescriptorPool &get() const noexcept;

private:
  vk::raii::DescriptorPool pool_;
};

} // namespace engine
