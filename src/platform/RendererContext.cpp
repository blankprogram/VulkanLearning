

#include "engine/platform/RendererContext.hpp"
#include "engine/utils/VulkanHelpers.hpp"
#include <cstring>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#define VMA_IMPLEMENTATION
#include "externals/vk_mem_alloc.h"
#define IMGUI_IMPL_VULKAN_NO_PROTOTYPES

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
RendererContext::RendererContext(GLFWwindow *window)
    : cam_(glm::radians(45.0f), 1280.f / 720.f, 0.1f, 1000.0f) {
  init(window);
}

RendererContext::~RendererContext() { cleanup(); }

void RendererContext::init(GLFWwindow *window) {
  device_ = std::make_unique<VulkanDevice>(window);
  swapchain_ =
      std::make_unique<Swapchain>(device_.get(), device_->getSurface(), window);
  allocator_ = device_->getAllocator();

  frameSync_.init(device_->getDevice(), MAX_FRAMES_IN_FLIGHT);
  commandManager_.init(device_->getDevice(),
                       device_->getGraphicsQueueFamilyIndex(),
                       MAX_FRAMES_IN_FLIGHT);
  renderResources_.init(device_.get(), swapchain_.get());
}

void RendererContext::beginFrame() {
  VkDevice dev = device_->getDevice();
  VkFence fence = frameSync_.getInFlightFence(currentFrame_);
  vkWaitForFences(dev, 1, &fence, VK_TRUE, UINT64_MAX);
  vkResetFences(dev, 1, &fence);

  VkResult result =
      vkAcquireNextImageKHR(dev, swapchain_->getSwapchain(), UINT64_MAX,
                            frameSync_.getImageAvailable(currentFrame_),
                            VK_NULL_HANDLE, &currentImageIndex_);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    renderGraph_.endFrame();
    return;
  }

  glm::mat4 viewProj = cam_.viewProjection();

  VkImageLayout layout = renderGraph_.getFinalLayout(); // from previous frame
  renderGraph_.beginFrame(
      renderResources_, commandManager_, currentFrame_, currentImageIndex_,
      viewProj,
      renderResources_.getSwapchain()->getImages()[currentImageIndex_], layout);
}

void RendererContext::endFrame() {
  renderGraph_.endFrame();
  renderGraph_.reset();

  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  VkSemaphore waitSems[] = {frameSync_.getImageAvailable(currentFrame_)};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore signalSems[] = {frameSync_.getRenderFinished(currentFrame_)};

  submit.waitSemaphoreCount = 1;
  submit.pWaitSemaphores = waitSems;
  submit.pWaitDstStageMask = waitStages;
  submit.commandBufferCount = 1;
  submit.pCommandBuffers = &renderGraph_.getCurrentCommandBuffer();
  submit.signalSemaphoreCount = 1;
  submit.pSignalSemaphores = signalSems;

  vkQueueSubmit(device_->getGraphicsQueue(), 1, &submit,
                frameSync_.getInFlightFence(currentFrame_));

  VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  present.waitSemaphoreCount = 1;
  present.pWaitSemaphores = signalSems;
  present.swapchainCount = 1;
  VkSwapchainKHR sc = swapchain_->getSwapchain();
  present.pSwapchains = &sc;
  present.pImageIndices = &currentImageIndex_;

  VkResult result = vkQueuePresentKHR(device_->getGraphicsQueue(), &present);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
    renderGraph_.reset();
    recreateSwapchain();
  }

  currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RendererContext::recreateSwapchain() {
  vkDeviceWaitIdle(device_->getDevice());
  swapchain_->recreate();
  cam_.setAspect(float(swapchain_->getExtent().width) /
                 float(swapchain_->getExtent().height));
  renderResources_.recreate();
}

void RendererContext::cleanup() {
  vkDeviceWaitIdle(device_->getDevice());

  cleanupImGui(); // ✅

  renderResources_.cleanup();
  commandManager_.cleanup(device_->getDevice());
  frameSync_.cleanup(device_->getDevice());
  swapchain_->cleanup();
  vmaDestroyAllocator(allocator_);
  device_.reset();
}

void RendererContext::initImGui(GLFWwindow *window) {
  // 1. Create ImGui Descriptor Pool
  VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
  };
  VkDescriptorPoolCreateInfo pool_info{};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1000;
  pool_info.poolSizeCount = 1;
  pool_info.pPoolSizes = pool_sizes;

  if (vkCreateDescriptorPool(device_->getDevice(), &pool_info, nullptr,
                             &imguiDescriptorPool_) != VK_SUCCESS)
    throw std::runtime_error("Failed to create ImGui descriptor pool");

  // 2. Setup ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();

  // 3. Init backends
  ImGui_ImplGlfw_InitForVulkan(window, true);

  ImGui_ImplVulkan_InitInfo init_info{};
  init_info.Instance = device_->getInstance();
  init_info.PhysicalDevice = device_->getPhysicalDevice();
  init_info.Device = device_->getDevice();
  init_info.QueueFamily = device_->getGraphicsQueueFamilyIndex();
  init_info.Queue = device_->getGraphicsQueue();
  init_info.DescriptorPool = imguiDescriptorPool_;
  init_info.MinImageCount = 2;
  init_info.ImageCount = 2;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.RenderPass = renderResources_.getRenderPass(); // ← ✅ THIS LINE
  init_info.CheckVkResultFn = nullptr;

  ImGui_ImplVulkan_Init(&init_info);

  // 4. Upload fonts
  VkCommandBuffer cmd = commandManager_.get(0); // or one-time buffer
  vkResetCommandBuffer(cmd, 0);
  VkCommandBufferBeginInfo beginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd, &beginInfo);
  ImGui_ImplVulkan_CreateFontsTexture();
  vkEndCommandBuffer(cmd);

  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmd;
  vkQueueSubmit(device_->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(device_->getGraphicsQueue());
  ImGui_ImplVulkan_DestroyFontsTexture(); // ✅ Correct function
}

void RendererContext::cleanupImGui() {
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  if (imguiDescriptorPool_ != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device_->getDevice(), imguiDescriptorPool_,
                            nullptr);
    imguiDescriptorPool_ = VK_NULL_HANDLE;
  }
}
