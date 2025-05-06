#include "engine/pipeline/ShaderModule.hpp"
#include <fstream>
#include <stdexcept>
#include <vector>

namespace {

static std::vector<uint32_t> loadFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open shader file: " + filename);
  }
  size_t size = file.tellg();
  std::vector<uint32_t> buffer(size / sizeof(uint32_t));
  file.seekg(0);
  file.read(reinterpret_cast<char *>(buffer.data()), size);
  return buffer;
}

static vk::raii::ShaderModule createModule(const vk::raii::Device &device,
                                           const std::string &filename) {
  auto code = loadFile(filename);
  vk::ShaderModuleCreateInfo ci({}, code.size(), code.data());
  return vk::raii::ShaderModule(device, ci);
}

} // anonymous namespace

namespace engine {

ShaderModule::ShaderModule(const vk::raii::Device &device,
                           const std::string &filename)
    : module_{createModule(device, filename)} {}

vk::PipelineShaderStageCreateInfo
ShaderModule::stageInfo(vk::ShaderStageFlagBits stage) const {
  return vk::PipelineShaderStageCreateInfo{}
      .setStage(stage)
      .setModule(*module_)
      .setPName("main");
}

} // namespace engine
