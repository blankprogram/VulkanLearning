#pragma once

#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class PipelineLayout {
public:
  PipelineLayout(const vk::raii::Device &device,
                 const std::vector<vk::DescriptorSetLayout> &setLayouts,
                 const std::vector<vk::PushConstantRange> &pushConstants = {});
  ~PipelineLayout() = default;
  PipelineLayout(const PipelineLayout &) = delete;
  PipelineLayout &operator=(const PipelineLayout &) = delete;
  PipelineLayout(PipelineLayout &&) = default;
  PipelineLayout &operator=(PipelineLayout &&) = default;

  const vk::raii::PipelineLayout &get() const { return layout_; }

private:
  static vk::PipelineLayoutCreateInfo
  makeLayoutInfo(const std::vector<vk::DescriptorSetLayout> &setLayouts,
                 const std::vector<vk::PushConstantRange> &pushConstants);

  vk::raii::PipelineLayout layout_;
};

} // namespace engine
