#include "engine/pipeline/PipelineLayout.hpp"

namespace engine {

vk::PipelineLayoutCreateInfo PipelineLayout::makeLayoutInfo(
    const std::vector<vk::DescriptorSetLayout> &setLayouts,
    const std::vector<vk::PushConstantRange> &pushConstants) {
  vk::PipelineLayoutCreateInfo info{};
  info.setSetLayoutCount(static_cast<uint32_t>(setLayouts.size()))
      .setPSetLayouts(setLayouts.data())
      .setPushConstantRangeCount(static_cast<uint32_t>(pushConstants.size()))
      .setPPushConstantRanges(pushConstants.data());
  return info;
}

PipelineLayout::PipelineLayout(
    const vk::raii::Device &device,
    const std::vector<vk::DescriptorSetLayout> &setLayouts,
    const std::vector<vk::PushConstantRange> &pushConstants)
    : layout_{device, makeLayoutInfo(setLayouts, pushConstants)} {}

} // namespace engine
