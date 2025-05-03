#pragma once

#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class DescriptorSetLayout {
public:
  DescriptorSetLayout(
      const vk::raii::Device &device,
      const std::vector<vk::DescriptorSetLayoutBinding> &bindings);

  const vk::raii::DescriptorSetLayout &get() const noexcept;

private:
  vk::raii::DescriptorSetLayout layout_;
};

} // namespace engine
