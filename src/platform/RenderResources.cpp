#include "engine/platform/RenderResources.hpp"
#include <string>

void RenderResources::init(VulkanDevice *device, Swapchain *swapchain) {
    device_ = device;
    swapchain_ = swapchain;
    allocator_ = device_->getAllocator();

    VkExtent2D extent = swapchain_->getExtent();
    VkFormat colorFormat = swapchain_->getImageFormat();

    depth_.init(device_->getDevice(), allocator_, device_->getPhysicalDevice(),
                extent, VK_FORMAT_D32_SFLOAT);

    renderPass_.init(device_->getDevice(), colorFormat, depth_.format());

    uniforms_.init(device_->getDevice(), allocator_, 2);

    pipeline_.init(device_->getDevice(), renderPass_.get(), uniforms_.layout(),
                   std::string(SPIRV_OUT) + "/vert.spv",
                   std::string(SPIRV_OUT) + "/frag.spv");

    framebuffers_.init(device_->getDevice(), renderPass_.get(), extent,
                       swapchain_->getImageViews(), depth_.view());
}

void RenderResources::recreate() {
    vkDeviceWaitIdle(device_->getDevice());

    VkExtent2D extent = swapchain_->getExtent();

    depth_.cleanup(device_->getDevice(), allocator_);
    depth_.init(device_->getDevice(), allocator_, device_->getPhysicalDevice(),
                extent, VK_FORMAT_D32_SFLOAT);

    renderPass_.cleanup(device_->getDevice());
    renderPass_.init(device_->getDevice(), swapchain_->getImageFormat(),
                     depth_.format());

    pipeline_.cleanup(device_->getDevice());
    pipeline_.init(device_->getDevice(), renderPass_.get(), uniforms_.layout(),
                   std::string(SPIRV_OUT) + "/vert.spv",
                   std::string(SPIRV_OUT) + "/frag.spv");

    framebuffers_.cleanup(device_->getDevice());
    framebuffers_.init(device_->getDevice(), renderPass_.get(), extent,
                       swapchain_->getImageViews(), depth_.view());
}

void RenderResources::cleanup() {
    framebuffers_.cleanup(device_->getDevice());
    renderPass_.cleanup(device_->getDevice());
    depth_.cleanup(device_->getDevice(), allocator_);
    uniforms_.cleanup(device_->getDevice(), allocator_);
    pipeline_.cleanup(device_->getDevice());
}

const engine::render::Pipeline &RenderResources::getPipeline() const {
    return pipeline_;
}

VkRenderPass RenderResources::getRenderPass() const {
    return renderPass_.get();
}

const std::vector<VkFramebuffer> &RenderResources::getFramebuffers() const {
    return framebuffers_.get();
}

const std::vector<VkDescriptorSet> &RenderResources::getDescriptorSets() const {
    return uniforms_.sets();
}

VkDescriptorSetLayout RenderResources::getDescriptorSetLayout() const {
    return uniforms_.layout();
}

Swapchain *RenderResources::getSwapchain() const { return swapchain_; }

void RenderResources::updateUniforms(size_t frameIndex,
                                     const glm::mat4 &viewProj) {
    uniforms_.update(allocator_, frameIndex, viewProj);
}

VmaAllocator RenderResources::getAllocator() const { return allocator_; }
