
#define GLFW_INCLUDE_VULKAN
#include "engine/platform/VulkanContext.hpp"
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

  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_, [](GLFWwindow *window, int, int) {
    auto app =
        reinterpret_cast<VulkanContext *>(glfwGetWindowUserPointer(window));
    app->framebufferResized_ = true;
  });
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
  createDepthResources();
  createRenderPass();

  createDescriptorSetLayout();

  createUniformBuffers();
  createDescriptorPool();
  createCommandPool(gf);

  texture_.Load(device_, physicalDevice_, commandPool_, graphicsQueue_,
                "assets/textures/cobble.jpg");
  createDescriptorSets();

  graphicsPipeline_.Init(
      device_, swapchainExtent_, renderPass_,
      descriptorSetLayout_ // pass your context’s descriptor‐set layout
  );

  // ← move createCommandPool up here, before any buffer copies:

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

  // destroy per-frame sync & uniform resources
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(device_, renderFinishedSemaphores_[i], nullptr);
    vkDestroySemaphore(device_, imageAvailableSemaphores_[i], nullptr);
    vkDestroyFence(device_, inFlightFences_[i], nullptr);

    vkDestroyBuffer(device_, uniformBuffers_[i], nullptr);

    vkDestroyImageView(device_, depthImageView_, nullptr);
    vkDestroyImage(device_, depthImage_, nullptr);
    vkFreeMemory(device_, depthImageMemory_, nullptr);
    vkFreeMemory(device_, uniformBuffersMemory_[i], nullptr);

    texture_.Cleanup(device_);
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

  std::vector<const char *> extensions(exts, exts + ec);
  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  ci.pApplicationInfo = &ai;
  ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  ci.ppEnabledExtensionNames = extensions.data();

  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};

  if (enableValidationLayers) {
    ci.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    ci.ppEnabledLayerNames = validationLayers.data();
  } else {
    ci.enabledLayerCount = 0;
  }

  CHECK_VK_RESULT(vkCreateInstance(&ci, nullptr, &instance_));

  if (enableValidationLayers) {
    setupDebugMessenger();
  }
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
  // 1) Set up the queue info exactly as before
  float priority = 1.0f;
  VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  qci.queueFamilyIndex = qf;
  qci.queueCount = 1;
  qci.pQueuePriorities = &priority;

  // 2) Query what the physical device supports
  VkPhysicalDeviceFeatures deviceFeatures{};
  vkGetPhysicalDeviceFeatures(physicalDevice_, &deviceFeatures);

  // 3) Enable sampler anisotropy if available
  if (deviceFeatures.samplerAnisotropy) {
    deviceFeatures.samplerAnisotropy = VK_TRUE;
  } else {
    std::cerr << "Warning: samplerAnisotropy not supported; disabling it.\n";
    deviceFeatures.samplerAnisotropy = VK_FALSE;
  }

  // 4) Fill in your VkDeviceCreateInfo, including pEnabledFeatures
  VkDeviceCreateInfo di{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  di.queueCreateInfoCount = 1;
  di.pQueueCreateInfos = &qci;
  di.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions_.size());
  di.ppEnabledExtensionNames = deviceExtensions_.data();
  di.pEnabledFeatures = &deviceFeatures; // ← make sure this is set!

  // 5) Create the device and grab the graphics queue
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
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapchainImageFormat_;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAttachment{};
  depthAttachment.format = VK_FORMAT_D32_SFLOAT;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef{};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef; // <-- important

  std::array<VkAttachmentDescription, 2> attachments = {colorAttachment,
                                                        depthAttachment};
  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo{
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1; // <--- ADD THIS
  renderPassInfo.pDependencies = &dependency;

  CHECK_VK_RESULT(
      vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_));
}

void VulkanContext::createFramebuffers() {
  swapchainFramebuffers_.resize(swapchainImageViews_.size());
  for (size_t i = 0; i < swapchainImageViews_.size(); ++i) {
    std::array<VkImageView, 2> attachments = {swapchainImageViews_[i],
                                              depthImageView_};

    VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fbInfo.renderPass = renderPass_;
    fbInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    fbInfo.pAttachments = attachments.data();
    fbInfo.width = swapchainExtent_.width;
    fbInfo.height = swapchainExtent_.height;
    fbInfo.layers = 1;

    CHECK_VK_RESULT(vkCreateFramebuffer(device_, &fbInfo, nullptr,
                                        &swapchainFramebuffers_[i]));
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
    VkCommandBuffer cmd = commandBuffers_[i];

    VkCommandBufferBeginInfo beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    CHECK_VK_RESULT(vkBeginCommandBuffer(cmd, &beginInfo));

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpbi.renderPass = renderPass_;
    rpbi.framebuffer = swapchainFramebuffers_[i];
    rpbi.renderArea.offset = {0, 0};
    rpbi.renderArea.extent = swapchainExtent_;
    rpbi.clearValueCount = static_cast<uint32_t>(clearValues.size());
    rpbi.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphicsPipeline_.GetPipeline());

    VkBuffer vertexBuffers[] = {vertexBuffer_};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

    vkCmdBindIndexBuffer(cmd, indexBuffer_, 0, VK_INDEX_TYPE_UINT16);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            graphicsPipeline_.GetLayout(), 0, 1,
                            &descriptorSets_[currentFrame_], 0, nullptr);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchainExtent_.width;
    viewport.height = (float)swapchainExtent_.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent_;

    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdDrawIndexed(cmd, indexCount_, 1, 0, 0, 0);

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

  VkResult result = vkQueuePresentKHR(graphicsQueue_, &presentInfo);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      framebufferResized_) {
    framebufferResized_ = false;
    recreateSwapchain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("Failed to present swap chain image!");
  }

  currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanContext::createVertexBuffer() {
  // Cube vertices

  std::vector<Vertex> vertices = {
      // Front face
      {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
      {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
      {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
      // Back face
      {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
      {{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
      {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
      {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
      // Left face
      {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
      {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
      {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
      {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
      // Right face
      {{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
      {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
      {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
      {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
      // Top face
      {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
      {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
      {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
      {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
      // Bottom face
      {{-0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
      {{0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
      {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
      {{-0.5f, -0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
  };

  std::vector<uint16_t> indices = {
      0,  1,  2,  2,  3,  0,  // Front
      4,  6,  5,  6,  4,  7,  // Back (flipped)
      8,  9,  10, 10, 11, 8,  // Left
      12, 14, 13, 14, 12, 15, // Right (flipped)
      16, 18, 17, 18, 16, 19, // Top (flipped)
      20, 21, 22, 22, 23, 20  // Bottom
  };

  vertexCount_ = static_cast<uint32_t>(vertices.size());
  indexCount_ = static_cast<uint32_t>(indices.size());

  VkDeviceSize vertexSize = sizeof(vertices[0]) * vertices.size();
  VkDeviceSize indexSize = sizeof(indices[0]) * indices.size();

  // Vertex buffer staging
  VkBuffer stagingVertexBuffer;
  VkDeviceMemory stagingVertexMemory;
  createBuffer(vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingVertexBuffer, stagingVertexMemory);

  void *data;
  vkMapMemory(device_, stagingVertexMemory, 0, vertexSize, 0, &data);
  memcpy(data, vertices.data(), (size_t)vertexSize);
  vkUnmapMemory(device_, stagingVertexMemory);

  createBuffer(
      vertexSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer_, vertexBufferMemory_);

  copyBuffer(stagingVertexBuffer, vertexBuffer_, vertexSize);

  vkDestroyBuffer(device_, stagingVertexBuffer, nullptr);
  vkFreeMemory(device_, stagingVertexMemory, nullptr);

  // Index buffer staging
  VkBuffer stagingIndexBuffer;
  VkDeviceMemory stagingIndexMemory;
  createBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               stagingIndexBuffer, stagingIndexMemory);

  vkMapMemory(device_, stagingIndexMemory, 0, indexSize, 0, &data);
  memcpy(data, indices.data(), (size_t)indexSize);
  vkUnmapMemory(device_, stagingIndexMemory);

  createBuffer(
      indexSize,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer_, indexBufferMemory_);

  copyBuffer(stagingIndexBuffer, indexBuffer_, indexSize);

  vkDestroyBuffer(device_, stagingIndexBuffer, nullptr);
  vkFreeMemory(device_, stagingIndexMemory, nullptr);
}

void VulkanContext::createDepthResources() {
  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT; // 32-bit float depth

  VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = swapchainExtent_.width;
  imageInfo.extent.height = swapchainExtent_.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = depthFormat;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  CHECK_VK_RESULT(vkCreateImage(device_, &imageInfo, nullptr, &depthImage_));

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device_, depthImage_, &memRequirements);

  VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  CHECK_VK_RESULT(
      vkAllocateMemory(device_, &allocInfo, nullptr, &depthImageMemory_));
  vkBindImageMemory(device_, depthImage_, depthImageMemory_, 0);

  VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  viewInfo.image = depthImage_;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = depthFormat;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  CHECK_VK_RESULT(
      vkCreateImageView(device_, &viewInfo, nullptr, &depthImageView_));
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

void VulkanContext::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

  endSingleTimeCommands(commandBuffer);
}

void VulkanContext::updateUniformBuffer(uint32_t frameIndex) {
  static auto start = std::chrono::high_resolution_clock::now();
  auto now = std::chrono::high_resolution_clock::now();
  float t = std::chrono::duration<float>(now - start).count();

  UBO ubo{};

  ubo.model = glm::rotate(glm::mat4(1.0f), t * glm::radians(45.0f),
                          glm::vec3(1.0f, 0.0f, 0.0f)); // rotate around X
  ubo.model = glm::rotate(ubo.model, t * glm::radians(30.0f),
                          glm::vec3(0.0f, 1.0f, 0.0f)); // rotate around Y
  ubo.model = glm::rotate(ubo.model, t * glm::radians(15.0f),
                          glm::vec3(0.0f, 0.0f, 1.0f)); // rotate around Z

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

  std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

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
  std::array<VkDescriptorPoolSize, 2> poolSizes{};

  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT;

  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = MAX_FRAMES_IN_FLIGHT;

  VkDescriptorPoolCreateInfo poolInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
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

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  imageInfo.imageView = texture_.GetImageView();
  imageInfo.sampler = texture_.GetSampler();

  std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

  descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[0].dstSet = descriptorSets_[idx];
  descriptorWrites[0].dstBinding = 0;
  descriptorWrites[0].dstArrayElement = 0;
  descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  descriptorWrites[0].descriptorCount = 1;
  descriptorWrites[0].pBufferInfo = &bufferInfo;

  descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrites[1].dstSet = descriptorSets_[idx];
  descriptorWrites[1].dstBinding = 1;
  descriptorWrites[1].dstArrayElement = 0;
  descriptorWrites[1].descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrites[1].descriptorCount = 1;
  descriptorWrites[1].pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(device_,
                         static_cast<uint32_t>(descriptorWrites.size()),
                         descriptorWrites.data(), 0, nullptr);
}

VkCommandBuffer VulkanContext::beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool_;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void VulkanContext::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue_);

  vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
}

void VulkanContext::recreateSwapchain() {
  int width = 0, height = 0;
  glfwGetFramebufferSize(window_, &width, &height);
  while (width == 0 || height == 0) {
    glfwGetFramebufferSize(window_, &width, &height);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device_);

  cleanupSwapchain();

  createSwapchain(findGraphicsQueueFamily());
  createImageViews();
  createDepthResources();
  createFramebuffers();

  // 🎯 ADD THIS:
  vkFreeCommandBuffers(device_, commandPool_, commandBuffers_.size(),
                       commandBuffers_.data());
  createCommandBuffers();
}

void VulkanContext::cleanupSwapchain() {
  for (auto framebuffer : swapchainFramebuffers_)
    vkDestroyFramebuffer(device_, framebuffer, nullptr);

  for (auto imageView : swapchainImageViews_)
    vkDestroyImageView(device_, imageView, nullptr);

  vkDestroyImageView(device_, depthImageView_, nullptr);
  vkDestroyImage(device_, depthImage_, nullptr);
  vkFreeMemory(device_, depthImageMemory_, nullptr);

  vkDestroySwapchainKHR(device_, swapchain_, nullptr);
}

void VulkanContext::setupDebugMessenger() {
  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr;

  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance_, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    CHECK_VK_RESULT(func(instance_, &createInfo, nullptr, &debugMessenger_));
  } else {
    throw std::runtime_error("failed to set up debug messenger!");
  }
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {

  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}
