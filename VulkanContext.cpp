
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

  // — after renderPass_ is valid:
  graphicsPipeline_.Init(device_, swapchainExtent_, renderPass_);

  createFramebuffers();
  createCommandPool(gf);
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
  vkDestroySemaphore(device_, renderFinishedSemaphore_, nullptr);
  vkDestroySemaphore(device_, imageAvailableSemaphore_, nullptr);

  for (auto fb : swapchainFramebuffers_)
    vkDestroyFramebuffer(device_, fb, nullptr);
  for (auto iv : swapchainImageViews_)
    vkDestroyImageView(device_, iv, nullptr);

  graphicsPipeline_.Cleanup(device_);

  vkDestroyRenderPass(device_, renderPass_, nullptr);
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
  VkCommandBufferAllocateInfo cbai{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  cbai.commandPool = commandPool_;
  cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cbai.commandBufferCount = (uint32_t)commandBuffers_.size();
  CHECK_VK_RESULT(
      vkAllocateCommandBuffers(device_, &cbai, commandBuffers_.data()));

  for (size_t i = 0; i < commandBuffers_.size(); ++i) {
    VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    vkBeginCommandBuffer(commandBuffers_[i], &bi);

    VkClearValue cv{{{0.0f, 0.0f, 0.0f, 1.0f}}};
    VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    rpbi.renderPass = renderPass_;
    rpbi.framebuffer = swapchainFramebuffers_[i];
    rpbi.renderArea.offset = {0, 0};
    rpbi.renderArea.extent = swapchainExtent_;
    rpbi.clearValueCount = 1;
    rpbi.pClearValues = &cv;

    vkCmdBeginRenderPass(commandBuffers_[i], &rpbi, VK_SUBPASS_CONTENTS_INLINE);

    // bind & draw the triangle
    vkCmdBindPipeline(commandBuffers_[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphicsPipeline_.GetPipeline());
    vkCmdDraw(commandBuffers_[i], 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffers_[i]);
    vkEndCommandBuffer(commandBuffers_[i]);
  }
}

void VulkanContext::createSyncObjects() {
  VkSemaphoreCreateInfo sci{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  CHECK_VK_RESULT(
      vkCreateSemaphore(device_, &sci, nullptr, &imageAvailableSemaphore_));
  CHECK_VK_RESULT(
      vkCreateSemaphore(device_, &sci, nullptr, &renderFinishedSemaphore_));
}

void VulkanContext::drawFrame() {
  uint32_t idx;
  vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                        imageAvailableSemaphore_, VK_NULL_HANDLE, &idx);

  VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  VkSemaphore waits[] = {imageAvailableSemaphore_};
  VkPipelineStageFlags stages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  si.waitSemaphoreCount = 1;
  si.pWaitSemaphores = waits;
  si.pWaitDstStageMask = stages;
  si.commandBufferCount = 1;
  si.pCommandBuffers = &commandBuffers_[idx];

  VkSemaphore signals[] = {renderFinishedSemaphore_};
  si.signalSemaphoreCount = 1;
  si.pSignalSemaphores = signals;

  vkQueueSubmit(graphicsQueue_, 1, &si, VK_NULL_HANDLE);

  VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  pi.waitSemaphoreCount = 1;
  pi.pWaitSemaphores = signals;
  pi.swapchainCount = 1;
  pi.pSwapchains = &swapchain_;
  pi.pImageIndices = &idx;
  vkQueuePresentKHR(graphicsQueue_, &pi);

  vkQueueWaitIdle(graphicsQueue_);
}
