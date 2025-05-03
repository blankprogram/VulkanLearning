#pragma once

#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class DescriptorSet {
public:
  DescriptorSet(const vk::raii::Device &device,
                const vk::raii::DescriptorPool &pool,
                const std::vector<vk::DescriptorSetLayout> &layouts) noexcept;

  const vk::raii::DescriptorSet &get() const noexcept;
  const vk::raii::DescriptorSets &getAll() const noexcept;

private:
  vk::raii::DescriptorSets sets_;
};

} // namespace engine
