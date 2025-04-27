
// VulkanContext.cpp
#define GLFW_INCLUDE_VULKAN
#include "VulkanContext.h"
#include <GLFW/glfw3.h>

#define CHECK_VK_RESULT(x)                                                     \
  if ((x) != VK_SUCCESS) {                                                     \
    std::cerr << "Vulkan error at line " << __LINE__ << std::endl;             \
    std::exit(EXIT_FAILURE);                                                   \
  }

VulkanContext::VulkanContext(uint32_t w, uint32_t h, const std::string &t)
    : width_(w), height_(h), title_(t) {
  initWindow();
  initVulkan();
}

VulkanContext::~VulkanContext() { cleanup(); }

void VulkanContext::Run() { mainLoop(); }

void VulkanContext::initWindow() {
  if (!glfwInit())
    throw std::runtime_error("Failed to init GLFW");
  if (auto *s = std::getenv("XDG_SESSION_TYPE");
      s && std::string(s) == "wayland")
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
  if (!window_)
    throw std::runtime_error("Failed to create window");
}

void VulkanContext::initVulkan() {
  createInstance();
  CHECK_VK_RESULT(
      glfwCreateWindowSurface(instance_, window_, nullptr, &surface_));
  pickPhysicalDevice();
  uint32_t gf = findGraphicsQueueFamily();
  createLogicalDevice(gf);
  createSwapchain(gf);
  createImageViews();
  createRenderPass();

  createDescriptorSetLayout();

  createUniformBuffers();
  createDescriptorPool();
  createDescriptorSets();

  graphicsPipeline_.Init(
      device_, swapchainExtent_, renderPass_,
      descriptorSetLayout_ // pass your context’s descriptor‐set layout
  );

  // ← move createCommandPool up here, before any buffer copies:
  createCommandPool(gf);

  // now it's safe to stage & upload your vertex buffer
  createVertexBuffer();

  createFramebuffers();
  createCommandBuffers();
  createSyncObjects();
}

void VulkanContext::mainLoop() {
  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();
    drawFrame();
  }
  vkDeviceWaitIdle(device_);
}

void VulkanContext::cleanup() {
  // wait for everything on the GPU to finish
  vkDeviceWaitIdle(device_);

  // destroy per-frame sync & uniform resources
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(device_, renderFinishedSemaphores_[i], nullptr);
    vkDestroySemaphore(device_, imageAvailableSemaphores_[i], nullptr);
    vkDestroyFence(device_, inFlightFences_[i], nullptr);

    vkDestroyBuffer(device_, uniformBuffers_[i], nullptr);
    vkFreeMemory(device_, uniformBuffersMemory_[i], nullptr);
  }

  // destroy descriptor‐set machinery
  vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
  vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);

  // framebuffers & image views
  for (auto fb : swapchainFramebuffers_)
    vkDestroyFramebuffer(device_, fb, nullptr);
  for (auto iv : swapchainImageViews_)
    vkDestroyImageView(device_, iv, nullptr);

  // pipeline & renderpass
  graphicsPipeline_.Cleanup(device_);
  vkDestroyRenderPass(device_, renderPass_, nullptr);

  // swapchain, command pool, device, surface, instance
  vkDestroySwapchainKHR(device_, swapchain_, nullptr);
  vkDestroyCommandPool(device_, commandPool_, nullptr);
  vkDestroyDevice(device_, nullptr);
  vkDestroySurfaceKHR(instance_, surface_, nullptr);
  vkDestroyInstance(instance_, nullptr);

  glfwDestroyWindow(window_);
  glfwTerminate();
}

void VulkanContext::createInstance() {
  VkApplicationInfo ai{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  ai.pApplicationName = title_.c_str();
  ai.apiVersion = VK_API_VERSION_1_3;

  uint32_t ec = 0;
  const char **exts = glfwGetRequiredInstanceExtensions(&ec);
  VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  ci.pApplicationInfo = &ai;
  ci.enabledExtensionCount = ec;
  ci.ppEnabledExtensionNames = exts;

  CHECK_VK_RESULT(vkCreateInstance(&ci, nullptr, &instance_));
}

void VulkanContext::pickPhysicalDevice() {
  uint32_t c = 0;
  vkEnumeratePhysicalDevices(instance_, &c, nullptr);
  if (!c)
    throw std::runtime_error("No GPU");
  std::vector<VkPhysicalDevice> devs(c);
  vkEnumeratePhysicalDevices(instance_, &c, devs.data());
  physicalDevice_ = devs[0];
}

uint32_t VulkanContext::findGraphicsQueueFamily() {
  uint32_t c = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &c, nullptr);
  std::vector<VkQueueFamilyProperties> qf(c);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &c, qf.data());
  for (uint32_t i = 0; i < c; ++i) {
    VkBool32 pres = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, i, surface_, &pres);
    if ((qf[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && pres)
      return i;
  }
  throw std::runtime_error("No gfx+present queue");
}

void VulkanContext::createLogicalDevice(uint32_t qf) {
  float p = 1.0f;
  VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  qci.queueFamilyIndex = qf;
  qci.queueCount = 1;
  qci.pQueuePriorities = &p;

  VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  di.queueCreateInfoCount = 1;
  di.pQueueCreateInfos = &qci;
  di.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions_.size());
  di.ppEnabledExtensionNames = deviceExtensions_.data();

  CHECK_VK_RESULT(vkCreateDevice(physicalDevice_, &di, nullptr, &device_));
  vkGetDeviceQueue(device_, qf, 0, &graphicsQueue_);
}

void VulkanContext::createSwapchain(uint32_t) {
  VkSurfaceCapabilitiesKHR caps;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &caps);

  uint32_t fc = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &fc, nullptr);
  std::vector<VkSurfaceFormatKHR> fmts(fc);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &fc,
                                       fmts.data());
  auto &fmt = fmts[0];

  swapchainImageFormat_ = fmt.format;
  swapchainExtent_ = (caps.currentExtent.width != UINT32_MAX)
                         ? caps.currentExtent
                         : VkExtent2D{width_, height_};

  VkSwapchainCreateInfoKHR sci{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  sci.surface = surface_;
  sci.minImageCount = caps.minImageCount + 1;
  sci.imageFormat = swapchainImageFormat_;
  sci.imageColorSpace = fmt.colorSpace;
  sci.imageExtent = swapchainExtent_;
  sci.imageArrayLayers = 1;
  sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  sci.preTransform = caps.currentTransform;
  sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  sci.clipped = VK_TRUE;

  CHECK_VK_RESULT(vkCreateSwapchainKHR(device_, &sci, nullptr, &swapchain_));
  uint32_t ic = 0;
  vkGetSwapchainImagesKHR(device_, swapchain_, &ic, nullptr);
  swapchainImages_.resize(ic);
  vkGetSwapchainImagesKHR(device_, swapchain_, &ic, swapchainImages_.data());
}

void VulkanContext::createImageViews() {
  swapchainImageViews_.resize(swapchainImages_.size());
  for (size_t i = 0; i < swapchainImages_.size(); ++i) {
    VkImageViewCreateInfo iv{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    iv.image = swapchainImages_[i];
    iv.viewType = VK_IMAGE_VIEW_TYPE_2D;
    iv.format = swapchainImageFormat_;
    iv.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    iv.subresourceRange.baseMipLevel = 0;
    iv.subresourceRange.levelCount = 1;
    iv.subresourceRange.baseArrayLayer = 0;
    iv.subresourceRange.layerCount = 1;
    CHECK_VK_RESULT(
        vkCreateImageView(device_, &iv, nullptr, &swapchainImageViews_[i]));
  }
}

void VulkanContext::createRenderPass() {
  VkAttachmentDescription att{};
  att.format = swapchainImageFormat_;
  att.samples = VK_SAMPLE_COUNT_1_BIT;
  att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference ar{};
  ar.attachment = 0;
  ar.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription sp{};
  sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  sp.colorAttachmentCount = 1;
  sp.pColorAttachments = &ar;

  VkRenderPassCreateInfo rpci{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  rpci.attachmentCount = 1;
  rpci.pAttachments = &att;
  rpci.subpassCount = 1;
  rpci.pSubpasses = &sp;

  CHECK_VK_RESULT(vkCreateRenderPass(device_, &rpci, nullptr, &renderPass_));
}

void VulkanContext::createFramebuffers() {
  swapchainFramebuffers_.resize(swapchainImageViews_.size());
  for (size_t i = 0; i < swapchainImageViews_.size(); ++i) {
    VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fb.renderPass = renderPass_;
    fb.attachmentCount = 1;
    fb.pAttachments = &swapchainImageViews_[i];
    fb.width = swapchainExtent_.width;
    fb.height = swapchainExtent_.height;
    fb.layers = 1;
    CHECK_VK_RESULT(
        vkCreateFramebuffer(device_, &fb, nullptr, &swapchainFramebuffers_[i]));
  }
}

void VulkanContext::createCommandPool(uint32_t gf) {
  VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  cp.queueFamilyIndex = gf;
  CHECK_VK_RESULT(vkCreateCommandPool(device_, &cp, nullptr, &commandPool_));
}

void VulkanContext::createCommandBuffers() {
  commandBuffers_.resize(swapchainFramebuffers_.size());

  VkCommandBufferAllocateInfo allocInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandPool = commandPool_;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = (uint32_t)commandBuffers_.size();

  CHECK_VK_RESULT(
      vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()));

  for (size_t i = 0; i < commandBuffers_.size(); ++i) {
    VkCommandBuffer cmd = commandBuffers_[i]; // **alias** for clarity

    VkCommandBufferBeginInfo beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    CHECK_VK_RESULT(vkBeginCommandBuffer(cmd, &beginInfo));

    VkClearValue clearColor{{{0.0f, 0.0f, 0.0f, 1.0f}}};
    VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpbi.renderPass = renderPass_;
    rpbi.framebuffer = swapchainFramebuffers_[i];
    rpbi.renderArea.offset = {0, 0};
    rpbi.renderArea.extent = swapchainExtent_;
    rpbi.clearValueCount = 1;
    rpbi.pClearValues = &clearColor;

    vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphicsPipeline_.GetPipeline());

    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicsPipeline_.GetLayout(), // your VkPipelineLayout
        0,                             // firstSet
        1, &descriptorSets_[currentFrame_], 0, nullptr);

    // bind our vertex buffer and draw N vertices
    VkBuffer bufs[] = {vertexBuffer_};
    VkDeviceSize offs[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, bufs, offs);
    vkCmdDraw(cmd, vertexCount_, 1, 0, 0);

    vkCmdEndRenderPass(cmd);
    CHECK_VK_RESULT(vkEndCommandBuffer(cmd));
  }
}

void VulkanContext::createSyncObjects() {
  VkSemaphoreCreateInfo semInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start signaled

  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    CHECK_VK_RESULT(vkCreateSemaphore(device_, &semInfo, nullptr,
                                      &imageAvailableSemaphores_[i]));
    CHECK_VK_RESULT(vkCreateSemaphore(device_, &semInfo, nullptr,
                                      &renderFinishedSemaphores_[i]));
    CHECK_VK_RESULT(
        vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFences_[i]));
  }
}

void VulkanContext::drawFrame() {
  vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE,
                  UINT64_MAX);
  vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);

  uint32_t imageIndex;
  vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                        imageAvailableSemaphores_[currentFrame_],
                        VK_NULL_HANDLE, &imageIndex);

  // update uniforms for this frame:
  updateUniformBuffer(currentFrame_);

  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores_[currentFrame_]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers_[imageIndex];
  VkSemaphore signalSemaphores[] = {renderFinishedSemaphores_[currentFrame_]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  CHECK_VK_RESULT(vkQueueSubmit(graphicsQueue_, 1, &submitInfo,
                                inFlightFences_[currentFrame_]));

  VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain_;
  presentInfo.pImageIndices = &imageIndex;

  vkQueuePresentKHR(graphicsQueue_, &presentInfo);

  currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanContext::createVertexBuffer() {
  // 1) define your triangle in C++
  std::vector<Vertex> vertices = {
      {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // bottom, red
      {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},  // top-right, green
      {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}  // top-left, blue
  };
  vertexCount_ = static_cast<uint32_t>(vertices.size());
  VkDeviceSize size = sizeof(vertices[0]) * vertices.size();

  // 2) staging buffer (CPU visible)
  VkBuffer stagingBuf;
  VkDeviceMemory stagingMem;
  createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingBuf, stagingMem);

  // 3) copy vertex data into it
  void *data = nullptr;
  vkMapMemory(device_, stagingMem, 0, size, 0, &data);
  memcpy(data, vertices.data(), (size_t)size);
  vkUnmapMemory(device_, stagingMem);

  // 4) create device-local buffer
  createBuffer(
      size,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer_, vertexBufferMemory_);

  // 5) copy from staging → device
  copyBuffer(stagingBuf, vertexBuffer_, size);

  // 6) clean up staging
  vkDestroyBuffer(device_, stagingBuf, nullptr);
  vkFreeMemory(device_, stagingMem, nullptr);
}

void VulkanContext::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags props, VkBuffer &buffer,
                                 VkDeviceMemory &mem) {
  VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
  bci.size = size;
  bci.usage = usage;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  CHECK_VK_RESULT(vkCreateBuffer(device_, &bci, nullptr, &buffer));

  VkMemoryRequirements mr;
  vkGetBufferMemoryRequirements(device_, buffer, &mr);

  VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  mai.allocationSize = mr.size;
  mai.memoryTypeIndex = findMemoryType(mr.memoryTypeBits, props);

  CHECK_VK_RESULT(vkAllocateMemory(device_, &mai, nullptr, &mem));
  vkBindBufferMemory(device_, buffer, mem, 0);
}

uint32_t VulkanContext::findMemoryType(uint32_t typeFilter,
                                       VkMemoryPropertyFlags props) const {
  VkPhysicalDeviceMemoryProperties memProps;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProps);
  for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
    if ((typeFilter & (1 << i)) &&
        (memProps.memoryTypes[i].propertyFlags & props) == props) {
      return i;
    }
  }
  throw std::runtime_error("Failed to find suitable memory type");
}

void VulkanContext::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize sz) {
  VkCommandBufferAllocateInfo cabi{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  cabi.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cabi.commandPool = commandPool_;
  cabi.commandBufferCount = 1;

  VkCommandBuffer cmd;
  vkAllocateCommandBuffers(device_, &cabi, &cmd);

  VkCommandBufferBeginInfo bb{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  bb.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmd, &bb);

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = sz;
  vkCmdCopyBuffer(cmd, src, dst, 1, &copyRegion);

  vkEndCommandBuffer(cmd);

  VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  si.commandBufferCount = 1;
  si.pCommandBuffers = &cmd;
  vkQueueSubmit(graphicsQueue_, 1, &si, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue_);

  vkFreeCommandBuffers(device_, commandPool_, 1, &cmd);
}

void VulkanContext::updateUniformBuffer(uint32_t frameIndex) {
  static auto start = std::chrono::high_resolution_clock::now();
  auto now = std::chrono::high_resolution_clock::now();
  float t = std::chrono::duration<float>(now - start).count();

  UBO ubo{};
  ubo.model = glm::rotate(glm::mat4(1.0f), t * glm::radians(90.0f),
                          glm::vec3(0.0f, 1.0f, 0.0f));
  ubo.view =
      glm::lookAt(glm::vec3(2, 2, 2), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));
  ubo.proj = glm::perspective(
      glm::radians(45.0f),
      swapchainExtent_.width / float(swapchainExtent_.height), 0.1f, 10.0f);
  ubo.proj[1][1] *= -1; // GLM → Vulkan

  void *data;
  vkMapMemory(device_, uniformBuffersMemory_[frameIndex], 0, sizeof(ubo), 0,
              &data);
  std::memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(device_, uniformBuffersMemory_[frameIndex]);
}

void VulkanContext::createDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding uboBinding{};
  uboBinding.binding = 0;
  uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboBinding.descriptorCount = 1;
  uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  uboBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutCreateInfo layoutInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &uboBinding;

  CHECK_VK_RESULT(vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr,
                                              &descriptorSetLayout_));
}

void VulkanContext::createUniformBuffers() {
  VkDeviceSize bufferSize = sizeof(UBO);
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniformBuffers_[i], uniformBuffersMemory_[i]);
  }
}

void VulkanContext::createDescriptorPool() {
  VkDescriptorPoolSize poolSize{};
  poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;

  VkDescriptorPoolCreateInfo poolInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

  CHECK_VK_RESULT(
      vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_));
}

void VulkanContext::createDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                             descriptorSetLayout_);
  VkDescriptorSetAllocateInfo allocInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  allocInfo.descriptorPool = descriptorPool_;
  allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
  allocInfo.pSetLayouts = layouts.data();

  CHECK_VK_RESULT(
      vkAllocateDescriptorSets(device_, &allocInfo, descriptorSets_.data()));
  // initial update
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    updateDescriptorSet(i);
}

void VulkanContext::updateDescriptorSet(uint32_t idx) {
  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = uniformBuffers_[idx];
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(UBO);

  VkWriteDescriptorSet descriptorWrite{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
  descriptorWrite.dstSet = descriptorSets_[idx];
  descriptorWrite.dstBinding = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = &bufferInfo;

  vkUpdateDescriptorSets(device_, 1, &descriptorWrite, 0, nullptr);
}
