#include "engine/ui/ImGuiLayer.hpp"
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>
#include <stdexcept>

using namespace engine;

ImGuiLayer::ImGuiLayer(GLFWwindow *window, VkInstance instance, VkDevice device,
                       VkPhysicalDevice physicalDevice,
                       uint32_t graphicsQueueFamily, VkQueue graphicsQueue,
                       vk::raii::DescriptorPool &descriptorPool,
                       VkRenderPass renderPass, uint32_t imageCount)
    : _window(window), _instance(instance), _device(device),
      _physical(physicalDevice), _queueFamily(graphicsQueueFamily),
      _queue(graphicsQueue), _descriptorPool(descriptorPool),
      _renderPass(renderPass), _imageCount(imageCount) {
  init();
}

ImGuiLayer::~ImGuiLayer() {
  vkDeviceWaitIdle(_device);
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void ImGuiLayer::init() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
#ifdef ImGuiConfigFlags_DockingEnable
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

  ImGui_ImplGlfw_InitForVulkan(_window, true);

  ImGui_ImplVulkan_InitInfo init_info{};
  init_info.Instance = _instance;
  init_info.PhysicalDevice = _physical;
  init_info.Device = _device;
  init_info.QueueFamily = _queueFamily;
  init_info.Queue = _queue;
  init_info.DescriptorPool = (VkDescriptorPool)*_descriptorPool;
  init_info.MinImageCount = 2;
  init_info.ImageCount = _imageCount;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = nullptr;
  init_info.RenderPass = _renderPass;
  init_info.Subpass = 0;

  if (!ImGui_ImplVulkan_Init(&init_info))
    throw std::runtime_error("Failed to initialize ImGui Vulkan backend");

  {
    VkCommandPoolCreateInfo pool_info{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = _queueFamily;
    VkCommandPool tmpPool;
    vkCreateCommandPool(_device, &pool_info, nullptr, &tmpPool);

    VkCommandBufferAllocateInfo alloc_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = tmpPool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(_device, &alloc_info, &cmd);

    VkCommandBufferBeginInfo begin_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin_info);

    // new signature: no args
    ImGui_ImplVulkan_CreateFontsTexture();

    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    vkQueueSubmit(_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(_queue);

    // new name in recent versions
    ImGui_ImplVulkan_DestroyFontsTexture();

    vkFreeCommandBuffers(_device, tmpPool, 1, &cmd);
    vkDestroyCommandPool(_device, tmpPool, nullptr);
  }
}

void ImGuiLayer::newFrame() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  // simple FPS counter
  double now = glfwGetTime();
  _frameCount++;
  if (now - _lastTime >= 1.0) {
    ImGui::Begin("Stats");
    ImGui::Text("FPS: %.1f", double(_frameCount) / (now - _lastTime));
    ImGui::End();
    _frameCount = 0;
    _lastTime = now;
  }
}

void ImGuiLayer::recreate(uint32_t imageCount, VkRenderPass renderPass) {
  // update stored values
  _imageCount = imageCount;
  _renderPass = renderPass;

  // shut down only the Vulkan backend
  ImGui_ImplVulkan_Shutdown();

  // re‑init Vulkan backend with the new swapchain parameters
  ImGui_ImplVulkan_InitInfo init_info{};
  init_info.Instance = _instance;
  init_info.PhysicalDevice = _physical;
  init_info.Device = _device;
  init_info.QueueFamily = _queueFamily;
  init_info.Queue = _queue;
  init_info.DescriptorPool = (VkDescriptorPool)*_descriptorPool;
  init_info.MinImageCount = 2; // your min frames in flight
  init_info.ImageCount = _imageCount;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = nullptr;
  init_info.RenderPass = _renderPass;
  init_info.Subpass = 0;

  if (!ImGui_ImplVulkan_Init(&init_info)) {
    throw std::runtime_error("Failed to re‑initialize ImGui Vulkan backend");
  }

  // recreate font textures (same as in init)
  {
    // one‑time command pool
    VkCommandPoolCreateInfo pool_info{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = _queueFamily;
    VkCommandPool tmpPool;
    vkCreateCommandPool(_device, &pool_info, nullptr, &tmpPool);

    VkCommandBufferAllocateInfo alloc_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = tmpPool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(_device, &alloc_info, &cmd);
    VkCommandBufferBeginInfo begin_info{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &begin_info);

    ImGui_ImplVulkan_CreateFontsTexture();
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    vkQueueSubmit(_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(_queue);

    ImGui_ImplVulkan_DestroyFontsTexture();
    vkFreeCommandBuffers(_device, tmpPool, 1, &cmd);
    vkDestroyCommandPool(_device, tmpPool, nullptr);
  }
}

void ImGuiLayer::render(VkCommandBuffer cmd) {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}
