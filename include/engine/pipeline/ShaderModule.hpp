#pragma once

#include <string>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class ShaderModule {
public:
  ShaderModule(const vk::raii::Device &device, const std::string &filename);

  vk::PipelineShaderStageCreateInfo
  stageInfo(vk::ShaderStageFlagBits stage) const;

private:
  vk::raii::ShaderModule module_;
};
} // namespace engine
