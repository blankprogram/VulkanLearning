

#pragma once

#include "engine/platform/DepthResources.hpp"
#include "engine/platform/FramebufferManager.hpp"
#include "engine/platform/RenderPassManager.hpp"
#include "engine/platform/Swapchain.hpp"
#include "engine/platform/UniformManager.hpp"
#include "engine/platform/VulkanDevice.hpp"
#include "engine/render/Pipeline.hpp"

class RenderResources {
  public:
    void init(VulkanDevice *device, Swapchain *swapchain);
    void recreate();
    void cleanup();

    const engine::render::Pipeline &getPipeline() const;
    VkRenderPass getRenderPass() const;
    const std::vector<VkFramebuffer> &getFramebuffers() const;
    const std::vector<VkDescriptorSet> &getDescriptorSets() const;
    VkDescriptorSetLayout getDescriptorSetLayout() const;

    VmaAllocator getAllocator() const;

    Swapchain *getSwapchain() const;
    void updateUniforms(size_t frameIndex, const glm::mat4 &viewProj);

  private:
    VulkanDevice *device_ = nullptr;
    Swapchain *swapchain_ = nullptr;
    VmaAllocator allocator_ = VK_NULL_HANDLE;

    DepthResources depth_;
    RenderPassManager renderPass_;
    UniformManager uniforms_;
    FramebufferManager framebuffers_;
    engine::render::Pipeline pipeline_;
};
