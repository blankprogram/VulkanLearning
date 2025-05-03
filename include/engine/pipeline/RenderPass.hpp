#pragma once

#include <vulkan/vulkan_raii.hpp>
namespace engine {

class RenderPass {
public:
  RenderPass(const vk::raii::Device &device, vk::Format colorFormat);

  ~RenderPass() = default;
  RenderPass(const RenderPass &) = delete;
  RenderPass &operator=(const RenderPass &) = delete;
  RenderPass(RenderPass &&) = default;
  RenderPass &operator=(RenderPass &&) = default;

  const vk::raii::RenderPass &get() const { return renderPass_; }

private:
  vk::raii::RenderPass renderPass_;
};

} // namespace engine
