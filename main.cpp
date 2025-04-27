#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>
#define CHECK_VK_RESULT(x)                                                     \
  if ((x) != VK_SUCCESS) {                                                     \
    std::cerr << "Vulkan error at line " << __LINE__ << std::endl;             \
    std::exit(EXIT_FAILURE);                                                   \
  }

GLFWwindow *window = nullptr;
VkInstance instance;
VkSurfaceKHR surface;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice device;
VkQueue graphicsQueue;
VkSwapchainKHR swapchain;
VkFormat swapchainImageFormat;
VkExtent2D swapchainExtent;
std::vector<VkImage> swapchainImages;
std::vector<VkImageView> swapchainImageViews;
std::vector<VkFramebuffer> swapchainFramebuffers;
VkRenderPass renderPass;
VkCommandPool commandPool;
std::vector<VkCommandBuffer> commandBuffers;
VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

void initWindow();
void createInstance();
void pickPhysicalDevice();
uint32_t findGraphicsQueueFamily();
void createLogicalDevice(uint32_t graphicsFamily);
void createSwapchain(uint32_t graphicsFamily);
void createImageViews();
void createRenderPass();
void createFramebuffers();
void createCommandPool(uint32_t graphicsFamily);
void createCommandBuffers();
void createSyncObjects();
void drawFrame();
void mainLoop();
void cleanup();

// --- Functions Implementation ---

// (Previous functions from initial steps...)

void createFramebuffers() {
  swapchainFramebuffers.resize(swapchainImageViews.size());

  for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
    VkFramebufferCreateInfo framebufferInfo{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = &swapchainImageViews[i];
    framebufferInfo.width = swapchainExtent.width;
    framebufferInfo.height = swapchainExtent.height;
    framebufferInfo.layers = 1;

    CHECK_VK_RESULT(vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                                        &swapchainFramebuffers[i]));
  }
}

void createCommandPool(uint32_t graphicsFamily) {
  VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  poolInfo.queueFamilyIndex = graphicsFamily;
  poolInfo.flags = 0;

  CHECK_VK_RESULT(
      vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool));
}

void createCommandBuffers() {
  commandBuffers.resize(swapchainFramebuffers.size());

  VkCommandBufferAllocateInfo allocInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
  CHECK_VK_RESULT(
      vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()));

  for (size_t i = 0; i < commandBuffers.size(); ++i) {
    VkCommandBufferBeginInfo beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    CHECK_VK_RESULT(vkBeginCommandBuffer(commandBuffers[i], &beginInfo));

    VkClearValue clearColor = {{{1.0f, 0.0f, 0.0f, 1.0f}}}; // Flat Red

    VkRenderPassBeginInfo renderPassInfo{
        VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapchainFramebuffers[i];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent;
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(commandBuffers[i]);

    CHECK_VK_RESULT(vkEndCommandBuffer(commandBuffers[i]));
  }
}

void createSyncObjects() {
  VkSemaphoreCreateInfo semaphoreInfo{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  CHECK_VK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                                    &imageAvailableSemaphore));
  CHECK_VK_RESULT(vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                                    &renderFinishedSemaphore));
}

void drawFrame() {
  uint32_t imageIndex;
  CHECK_VK_RESULT(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
                                        imageAvailableSemaphore, VK_NULL_HANDLE,
                                        &imageIndex));

  VkSubmitInfo submitInfo{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

  VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  CHECK_VK_RESULT(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));

  VkPresentInfoKHR presentInfo{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain;
  presentInfo.pImageIndices = &imageIndex;

  CHECK_VK_RESULT(vkQueuePresentKHR(graphicsQueue, &presentInfo));

  vkQueueWaitIdle(graphicsQueue);
}

void initWindow() {
  if (!glfwInit())
    throw std::runtime_error("Failed to initialize GLFW");

  if (const char *session = std::getenv("XDG_SESSION_TYPE");
      session && std::string(session) == "wayland")
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(800, 600, "Vulkan Red", nullptr, nullptr);
  if (!window)
    throw std::runtime_error("Failed to create GLFW window");
}

void createInstance() {
  VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.pApplicationName = "Vulkan Red";
  appInfo.apiVersion = VK_API_VERSION_1_3;

  uint32_t glfwExtensionCount;
  const char **glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  VkInstanceCreateInfo createInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = glfwExtensionCount;
  createInfo.ppEnabledExtensionNames = glfwExtensions;

  CHECK_VK_RESULT(vkCreateInstance(&createInfo, nullptr, &instance));
}

void pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0)
    throw std::runtime_error("No Vulkan compatible GPU found");

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
  physicalDevice = devices[0];
}

uint32_t findGraphicsQueueFamily() {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilies.data());

  for (uint32_t i = 0; i < queueFamilyCount; ++i) {
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
                                         &presentSupport);
    if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport)
      return i;
  }

  throw std::runtime_error("Failed to find a suitable queue family");
}

void createLogicalDevice(uint32_t graphicsFamily) {
  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo{
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queueCreateInfo.queueFamilyIndex = graphicsFamily;
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  VkDeviceCreateInfo createInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  createInfo.queueCreateInfoCount = 1;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  CHECK_VK_RESULT(
      vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));
  vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
}

void createSwapchain(uint32_t graphicsFamily) {
  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                            &capabilities);

  VkSurfaceFormatKHR format;
  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                       nullptr);
  std::vector<VkSurfaceFormatKHR> formats(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount,
                                       formats.data());
  format = formats[0];

  swapchainImageFormat = format.format;
  swapchainExtent = (capabilities.currentExtent.width != UINT32_MAX)
                        ? capabilities.currentExtent
                        : VkExtent2D{800, 600};

  VkSwapchainCreateInfoKHR swapchainInfo{
      VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  swapchainInfo.surface = surface;
  swapchainInfo.minImageCount = capabilities.minImageCount + 1;
  swapchainInfo.imageFormat = swapchainImageFormat;
  swapchainInfo.imageColorSpace = format.colorSpace;
  swapchainInfo.imageExtent = swapchainExtent;
  swapchainInfo.imageArrayLayers = 1;
  swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainInfo.preTransform = capabilities.currentTransform;
  swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  swapchainInfo.clipped = VK_TRUE;

  CHECK_VK_RESULT(
      vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &swapchain));

  uint32_t imageCount;
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
  swapchainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount,
                          swapchainImages.data());
}

void createImageViews() {
  swapchainImageViews.resize(swapchainImages.size());

  for (size_t i = 0; i < swapchainImages.size(); ++i) {
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = swapchainImages[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = swapchainImageFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    CHECK_VK_RESULT(
        vkCreateImageView(device, &viewInfo, nullptr, &swapchainImageViews[i]));
  }
}

void createRenderPass() {
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapchainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkRenderPassCreateInfo renderPassInfo{
      VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  CHECK_VK_RESULT(
      vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
}

void mainLoop() {
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    drawFrame();
  }

  vkDeviceWaitIdle(device);
}

void cleanup() {
  vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
  vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);

  for (auto framebuffer : swapchainFramebuffers)
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  for (auto view : swapchainImageViews)
    vkDestroyImageView(device, view, nullptr);

  vkDestroyRenderPass(device, renderPass, nullptr);
  vkDestroySwapchainKHR(device, swapchain, nullptr);
  vkDestroyCommandPool(device, commandPool, nullptr);
  vkDestroyDevice(device, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);
  glfwDestroyWindow(window);
  glfwTerminate();
}

int main() {
  try {
    initWindow();
    createInstance();
    CHECK_VK_RESULT(
        glfwCreateWindowSurface(instance, window, nullptr, &surface));
    pickPhysicalDevice();
    uint32_t graphicsFamily = findGraphicsQueueFamily();
    createLogicalDevice(graphicsFamily);
    createSwapchain(graphicsFamily);
    createImageViews();
    createRenderPass();
    createFramebuffers();
    createCommandPool(graphicsFamily);
    createCommandBuffers();
    createSyncObjects();
    mainLoop();
    cleanup();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
