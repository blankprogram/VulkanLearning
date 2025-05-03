
#pragma once

#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace engine::swapchain {

class Framebuffer {
public:
  Framebuffer(const vk::raii::Device &device,
              const vk::raii::RenderPass &renderPass, vk::Extent2D extent,
              const std::vector<vk::ImageView> &attachments);

  ~Framebuffer() = default;
  Framebuffer(const Framebuffer &) = delete;
  Framebuffer &operator=(const Framebuffer &) = delete;
  Framebuffer(Framebuffer &&) = default;
  Framebuffer &operator=(Framebuffer &&) = default;

  const vk::raii::Framebuffer &get() const { return framebuffer_; }

private:
  vk::raii::Framebuffer framebuffer_;
};

} // namespace engine::swapchain
