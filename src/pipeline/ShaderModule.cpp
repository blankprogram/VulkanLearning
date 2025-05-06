#include "engine/pipeline/ShaderModule.hpp"
#include <fstream>
#include <stdexcept>
#include <vector>

namespace engine {

static std::vector<char> readSPIRV(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open shader file: " + filename);
  }

  size_t fileSize = static_cast<size_t>(file.tellg());
  if (fileSize == 0 || (fileSize % 4) != 0) {
    throw std::runtime_error("SPIR-V file size not a multiple of 4: " +
                             filename);
  }

  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  return buffer;
}

ShaderModule::ShaderModule(const vk::raii::Device &device,
                           const std::string &filename)
    : module_{[&]() {
        auto codeBytes = readSPIRV(filename);
        vk::ShaderModuleCreateInfo createInfo{};
        createInfo.setCodeSize(codeBytes.size());
        createInfo.setPCode(
            reinterpret_cast<const uint32_t *>(codeBytes.data()));
        return vk::raii::ShaderModule(device, createInfo);
      }()} {}

vk::PipelineShaderStageCreateInfo
ShaderModule::stageInfo(vk::ShaderStageFlagBits stage) const {
  return vk::PipelineShaderStageCreateInfo{}
      .setStage(stage)
      .setModule(*module_)
      .setPName("main");
}

} // namespace engine
