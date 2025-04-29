
#define GLFW_INCLUDE_VULKAN
#include "engine/platform/VulkanContext.hpp"
#include <GLFW/glfw3.h>
#include <random>
#define CHECK_VK_RESULT(x)                                                     \
  if ((x) != VK_SUCCESS) {                                                     \
    std::cerr << "Vulkan error at line " << __LINE__ << std::endl;             \
    std::exit(EXIT_FAILURE);                                                   \
  }

VulkanContext::VulkanContext(uint32_t w, uint32_t h, const std::string &t)
    : width_(w), height_(h), title_(t), cameraAspect_(float(w) / float(h)) {
  initWindow();
  initVulkan();

  startWorkerThreads();

  // --- create Renderer (needs Vulkan stuff ready first) ---
  renderer_ = std::make_unique<Renderer>(device_, renderPass_,
                                         globalDescriptorSetLayout_,
                                         filledPipeline_, wireframePipeline_);

  // --- create Camera entity ---
  auto camE = registry_.create();
  registry_.emplace<CameraComponent>(camE, cameraAspect_);

  // --- NOW generate world terrain chunks! ---

  // If you still want to keep a small test volume, you can (optional)

  materials_.emplace_back(std::make_unique<Material>(
      device_, descriptorPool_, materialDescriptorSetLayout_, &texture_));

  initChunks(); // ✅ ✅ ✅ ADD THIS LINE
}

VulkanContext::~VulkanContext() {
  stopWorkerThreads();
  cleanup();
}

void VulkanContext::Run() { mainLoop(); }

void VulkanContext::initWindow() {
  // 1) initialize GLFW, create a no-API window
  if (!glfwInit())
    throw std::runtime_error("Failed to init GLFW");
  if (auto *s = std::getenv("XDG_SESSION_TYPE");
      s && std::string(s) == "wayland")
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window_ = glfwCreateWindow(width_, height_, title_.c_str(), nullptr, nullptr);
  if (!window_)
    throw std::runtime_error("Failed to create window");

  // 2) hide & capture the cursor so we get relative mouse motion
  glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  // 3) stash `this` pointer and install callbacks
  glfwSetWindowUserPointer(window_, this);

  // resize callback (unchanged)
  glfwSetFramebufferSizeCallback(window_, [](GLFWwindow *w, int width,
                                             int height) {
    auto app = reinterpret_cast<VulkanContext *>(glfwGetWindowUserPointer(w));
    app->onResize(width, height);
  });

  // mouse-move callback
  glfwSetCursorPosCallback(window_, [](GLFWwindow *w, double xpos,
                                       double ypos) {
    auto app = reinterpret_cast<VulkanContext *>(glfwGetWindowUserPointer(w));
    app->onMouseMoved(static_cast<float>(xpos), static_cast<float>(ypos));
  });
}

void VulkanContext::onMouseMoved(float xpos, float ypos) {
  if (firstMouse_) {
    lastX_ = xpos;
    lastY_ = ypos;
    firstMouse_ = false;
  }
  float xoffset = xpos - lastX_;
  float yoffset =
      lastY_ - ypos; // reversed: y-coordinates go from bottom to top
  lastX_ = xpos;
  lastY_ = ypos;

  auto view = registry_.view<CameraComponent>();
  for (auto e : view) {
    view.get<CameraComponent>(e).cam.ProcessMouseMovement(xoffset, yoffset);
  }
}

void VulkanContext::processInput(float dt) {
  // if either shift is held, double the effective delta-time
  bool fast = glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
              glfwGetKey(window_, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
  float speedDt = dt * (fast ? 2.0f : 1.0f);

  auto view = registry_.view<CameraComponent>();
  for (auto e : view) {
    auto &cam = view.get<CameraComponent>(e).cam;
    if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS)
      cam.ProcessKeyboard(FORWARD, speedDt);
    if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS)
      cam.ProcessKeyboard(BACKWARD, speedDt);
    if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS)
      cam.ProcessKeyboard(LEFT, speedDt);
    if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS)
      cam.ProcessKeyboard(RIGHT, speedDt);
  }
}

void VulkanContext::onResize(int newW, int newH) {
  framebufferResized_ = true;
  newWidth_ = newW;
  newHeight_ = newH;
  cameraAspect_ = float(newW) / float(newH);

  // update every CameraComponent in the registry:
  auto view = registry_.view<CameraComponent>();
  for (auto e : view) {
    view.get<CameraComponent>(e).cam.SetAspect(cameraAspect_);
  }
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

  createGlobalDescriptorSetLayout();
  createUniformBuffers();
  createDescriptorPool();
  createCommandPool(gf);

  texture_.Load(device_, physicalDevice_, commandPool_, graphicsQueue_,
                "assets/textures/cobble.png");
  createGlobalDescriptorSets();

  createMaterialDescriptorSetLayout();

  // pipelines now built with two sets: {global, material}
  filledPipeline_.Init(
      device_, swapchainExtent_, renderPass_, globalDescriptorSetLayout_,
      materialDescriptorSetLayout_, "assets/shaders/triangle.vert.spv",
      "assets/shaders/triangle.frag.spv", VK_POLYGON_MODE_FILL);

  wireframePipeline_.Init(
      device_, swapchainExtent_, renderPass_, globalDescriptorSetLayout_,
      materialDescriptorSetLayout_, "assets/shaders/triangle.vert.spv",
      "assets/shaders/wireframe.frag.spv", VK_POLYGON_MODE_LINE);

  createFramebuffers();
  createCommandBuffers();
  createSyncObjects();
}

void VulkanContext::mainLoop() {
  auto last = std::chrono::high_resolution_clock::now();
  while (!glfwWindowShouldClose(window_)) {
    auto now = std::chrono::high_resolution_clock::now();
    float dt = std::chrono::duration<float>(now - last).count();
    last = now;

    glfwPollEvents();
    processInput(dt);

    updateChunksAroundPlayer();

    {
      std::lock_guard<std::mutex> lk(doneMux_);
      while (!doneQueue_.empty()) {
        Chunk chunk = std::move(doneQueue_.front());
        doneQueue_.pop();
        // store into map, replacing placeholder
        chunks_[chunk.chunkPos] = std::move(chunk);

        // spawn ECS entity & attach Transform/MeshRef/MaterialRef
        auto e = registry_.create();
        const float voxelScale = 0.25f;
        const int chunkSize = 128;
        glm::vec3 worldPos = {chunk.chunkPos.x * chunkSize * voxelScale,
                              chunk.chunkPos.y * chunkSize * voxelScale,
                              chunk.chunkPos.z * chunkSize * voxelScale};
        registry_.emplace<Transform>(
            e, Transform{glm::translate(glm::mat4(1.0f), worldPos)});
        registry_.emplace<MeshRef>(e, chunks_[chunk.chunkPos].mesh.get());
        registry_.emplace<MaterialRef>(e, materials_.back().get());
      }
    }
    drawFrame();
  }
  vkDeviceWaitIdle(device_);
}

void VulkanContext::cleanup() {
  // make sure absolutely nothing is in flight
  vkDeviceWaitIdle(device_);

  // per-frame sync & UBOs
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device_, renderFinishedSemaphores_[i], nullptr);
    vkDestroySemaphore(device_, imageAvailableSemaphores_[i], nullptr);
    vkDestroyFence(device_, inFlightFences_[i], nullptr);

    vkDestroyBuffer(device_, uniformBuffers_[i], nullptr);
    vkFreeMemory(device_, uniformBuffersMemory_[i], nullptr);
  }

  texture_.Cleanup(device_);

  // descriptor sets/layouts
  vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
  vkDestroyDescriptorSetLayout(device_, globalDescriptorSetLayout_, nullptr);
  vkDestroyDescriptorSetLayout(device_, materialDescriptorSetLayout_, nullptr);

  // framebuffers & image-views
  for (auto fb : swapchainFramebuffers_)
    vkDestroyFramebuffer(device_, fb, nullptr);
  for (auto iv : swapchainImageViews_)
    vkDestroyImageView(device_, iv, nullptr);
  vkDestroyImageView(device_, depthImageView_, nullptr);
  vkDestroyImage(device_, depthImage_, nullptr);
  vkFreeMemory(device_, depthImageMemory_, nullptr);

  // pipelines & render pass
  filledPipeline_.Cleanup(device_);
  wireframePipeline_.Cleanup(device_);
  vkDestroyRenderPass(device_, renderPass_, nullptr);

  // swapchain, pools, device, surface, instance
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

void VulkanContext::createCommandPool(uint32_t gfxFam) {
  VkCommandPoolCreateInfo cp{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  cp.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  cp.queueFamilyIndex = gfxFam;
  CHECK_VK_RESULT(vkCreateCommandPool(device_, &cp, nullptr, &commandPool_));
}

void VulkanContext::createCommandBuffers() {
  commandBuffers_.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandPool = commandPool_;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = uint32_t(commandBuffers_.size());

  CHECK_VK_RESULT(
      vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()));
}

void VulkanContext::recordCommandBuffer(VkCommandBuffer cmd,
                                        uint32_t imageIndex,
                                        uint32_t frameIndex) {
  vkResetCommandBuffer(cmd, 0);
  VkCommandBufferBeginInfo bi{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  vkBeginCommandBuffer(cmd, &bi);

  std::array<VkClearValue, 2> clears{};
  clears[0].color = {{0, 0, 0, 1}};
  clears[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo rpbi{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  rpbi.renderPass = renderPass_;
  rpbi.framebuffer = swapchainFramebuffers_[imageIndex];
  rpbi.renderArea = {{0, 0}, swapchainExtent_};
  rpbi.clearValueCount = (uint32_t)clears.size();
  rpbi.pClearValues = clears.data();
  vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

  // choose pipeline & layout
  auto &pipeline = useWireframe_ ? wireframePipeline_ : filledPipeline_;
  VkPipelineLayout layout = pipeline.GetLayout();
  VkPipeline handle = pipeline.GetPipeline();

  // bind it:
  vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, handle);
  vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1,
                          &descriptorSets_[frameIndex], 0, nullptr);

  // now your Renderer needs to know which layout to use for push-constants &
  // materials, or you can pass the chosen layout into Renderer::Render as an
  // extra param.
  renderer_->Render(registry_, cmd, swapchainExtent_, descriptorSets_,
                    frameIndex, useWireframe_);

  vkCmdEndRenderPass(cmd);
  vkEndCommandBuffer(cmd);
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
  // 1) wait & reset fence
  vkWaitForFences(device_, 1, &inFlightFences_[currentFrame_], VK_TRUE,
                  UINT64_MAX);

  // 1a) handle resize
  if (framebufferResized_) {
    vkDeviceWaitIdle(device_);
    recreateSwapchain(uint32_t(newWidth_), uint32_t(newHeight_));
    cameraAspect_ = float(newWidth_) / float(newHeight_);
    framebufferResized_ = false;
  }

  vkResetFences(device_, 1, &inFlightFences_[currentFrame_]);

  // 2) acquire next image
  uint32_t imageIndex;
  vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX,
                        imageAvailableSemaphores_[currentFrame_],
                        VK_NULL_HANDLE, &imageIndex);

  // 3) update UBO
  updateUniformBuffer(currentFrame_);

  // 4) record commands
  VkCommandBuffer cmd = commandBuffers_[currentFrame_];
  recordCommandBuffer(cmd, imageIndex, currentFrame_);

  // 5) submit under lock
  VkSemaphore waitSems[] = {imageAvailableSemaphores_[currentFrame_]};
  VkPipelineStageFlags ws[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSemaphore signalSems[] = {renderFinishedSemaphores_[currentFrame_]};

  VkSubmitInfo si{VK_STRUCTURE_TYPE_SUBMIT_INFO};
  si.waitSemaphoreCount = 1;
  si.pWaitSemaphores = waitSems;
  si.pWaitDstStageMask = ws;
  si.commandBufferCount = 1;
  si.pCommandBuffers = &cmd;
  si.signalSemaphoreCount = 1;
  si.pSignalSemaphores = signalSems;

  {
    std::lock_guard<std::mutex> ql(queueMux_);
    CHECK_VK_RESULT(
        vkQueueSubmit(graphicsQueue_, 1, &si, inFlightFences_[currentFrame_]));
  }

  // 6) present under same lock
  VkPresentInfoKHR pi{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  pi.waitSemaphoreCount = 1;
  pi.pWaitSemaphores = signalSems;
  pi.swapchainCount = 1;
  pi.pSwapchains = &swapchain_;
  pi.pImageIndices = &imageIndex;

  VkResult res;
  {
    std::lock_guard<std::mutex> ql(queueMux_);
    res = vkQueuePresentKHR(graphicsQueue_, &pi);
  }
  if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
    framebufferResized_ = true;
  } else if (res != VK_SUCCESS) {
    throw std::runtime_error("Failed to present swapchain image!");
  }

  // 7) now that GPU is idle, destroy any deferred chunks
  {
    std::lock_guard<std::mutex> ql(queueMux_);
    vkQueueWaitIdle(graphicsQueue_);
  }
  pendingDestroy_.clear();

  // 8) next frame
  currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
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
  allocInfo.memoryTypeIndex =
      findMemoryType(physicalDevice_, memRequirements.memoryTypeBits,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

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

void VulkanContext::updateUniformBuffer(uint32_t frameIndex) {
  // grab the first camera we created
  auto view = registry_.view<CameraComponent>();
  if (view.empty())
    return;

  auto &cc = view.get<CameraComponent>(*view.begin());

  UBO ubo{};
  ubo.view = cc.cam.GetView();
  ubo.proj = cc.cam.GetProj();
  // model → per-entity via push-constant

  void *data = nullptr;
  vkMapMemory(device_, uniformBuffersMemory_[frameIndex], 0, sizeof(ubo), 0,
              &data);
  std::memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(device_, uniformBuffersMemory_[frameIndex]);
}

void VulkanContext::createGlobalDescriptorSetLayout() {
  std::array<VkDescriptorSetLayoutBinding, 2> bindings{};

  // binding 0 = UBO
  bindings[0].binding = 0;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].descriptorCount = 1;
  bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  // binding 1 = combined image sampler (per‐frame)
  bindings[1].binding = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[1].descriptorCount = 1;
  bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo info{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  info.bindingCount = static_cast<uint32_t>(bindings.size());
  info.pBindings = bindings.data();

  CHECK_VK_RESULT(vkCreateDescriptorSetLayout(device_, &info, nullptr,
                                              &globalDescriptorSetLayout_));
}

// — new: material‐only layout (set 1)
void VulkanContext::createMaterialDescriptorSetLayout() {
  VkDescriptorSetLayoutBinding b{};
  b.binding = 0;
  b.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  b.descriptorCount = 1;
  b.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo info{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  info.bindingCount = 1;
  info.pBindings = &b;

  CHECK_VK_RESULT(vkCreateDescriptorSetLayout(device_, &info, nullptr,
                                              &materialDescriptorSetLayout_));
}

// — allocate your *global* per-frame sets (set 0):
void VulkanContext::createGlobalDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                             globalDescriptorSetLayout_);
  VkDescriptorSetAllocateInfo alloc{
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
  alloc.descriptorPool = descriptorPool_;
  alloc.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
  alloc.pSetLayouts = layouts.data();

  CHECK_VK_RESULT(
      vkAllocateDescriptorSets(device_, &alloc, descriptorSets_.data()));
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    updateDescriptorSet(i);
}
void VulkanContext::createUniformBuffers() {
  VkDeviceSize bufferSize = sizeof(UBO);
  for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    createBuffer(device_, physicalDevice_, bufferSize,
                 VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 uniformBuffers_[i], uniformBuffersMemory_[i]);
  }
}

void VulkanContext::createDescriptorPool() {
  // We need one UBO set per in-flight frame, plus one combined-image-sampler
  // set for our cube material (adjust materialCount as you add more materials).
  const uint32_t materialCount = 1;

  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = MAX_FRAMES_IN_FLIGHT; // UBO sets

  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount =
      MAX_FRAMES_IN_FLIGHT + materialCount; // per-frame texture + material sets

  VkDescriptorPoolCreateInfo poolInfo{
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets =
      MAX_FRAMES_IN_FLIGHT + materialCount; // total descriptor sets

  CHECK_VK_RESULT(
      vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_));
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

void VulkanContext::recreateSwapchain(uint32_t width, uint32_t height) {
  // update the official extent
  swapchainExtent_ = {width, height};

  // destroy old swapchain stuff
  cleanupSwapchain();

  // re-create at the new size
  createSwapchain(findGraphicsQueueFamily());
  createImageViews();
  createDepthResources();
  createFramebuffers();

  // free & re-alloc your command buffers so your renderArea / scissor match
  vkFreeCommandBuffers(device_, commandPool_, uint32_t(commandBuffers_.size()),
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

Voxel VulkanContext::generateTerrainVoxel(int wx, int wy, int wz) {
  noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  float frequency = 0.02f;
  float amplitude = 20.0f;

  float n =
      std::fabs(noise.GetNoise((float)wx * frequency, (float)wz * frequency));
  n = glm::clamp(n, 0.0f, 1.0f);

  // make mountains sharper (optional tweak)
  n = std::pow(n, 1.5f);

  int groundHeight = 16 + int(amplitude * n);

  if (wy <= groundHeight) {
    static std::mt19937 rng(wx * 73856093 ^ wy * 19349663 ^ wz * 83492791);
    static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    float r = 0.4f + 0.2f * dist(rng);
    float g = 0.2f + 0.2f * dist(rng);
    float b = 0.1f + 0.1f * dist(rng);

    return Voxel{{r, g, b}, 1.0f, true};
  } else {
    return Voxel{{0.0f, 0.0f, 0.0f}, 1.0f, false};
  }
}

void VulkanContext::updateChunksAroundPlayer() {
  auto view = registry_.view<CameraComponent>();
  if (view.empty())
    return;

  auto &camera = view.get<CameraComponent>(*view.begin()).cam;

  glm::vec3 playerPos = camera.GetPosition();

  float voxelScale = 0.25f;
  int chunkSize = 128; // 32 voxels per chunk

  glm::ivec3 currentChunk = glm::floor(playerPos / (chunkSize * voxelScale));

  if (currentChunk == lastPlayerChunk_)
    return; // player still in same chunk, no need to update

  lastPlayerChunk_ = currentChunk;

  const int loadDistance = 2; // how many chunks to load around player

  std::unordered_set<glm::ivec3> desiredChunks;
  for (int dx = -loadDistance; dx <= loadDistance; ++dx) {
    for (int dz = -loadDistance; dz <= loadDistance; ++dz) {
      glm::ivec3 coord = currentChunk + glm::ivec3(dx, 0, dz);
      desiredChunks.insert(coord);

      if (chunks_.count(coord) == 0) {
        // placeholder
        chunks_[coord] = Chunk{};
        chunks_[coord].chunkPos = coord;
        // schedule a worker
        {
          std::lock_guard<std::mutex> lk(taskMux_);
          taskQueue_.push(coord);
        }
        taskCV_.notify_one();
      }
    }
  }

  // Now unload distant chunks
  std::vector<glm::ivec3> toDelete;
  for (const auto &[coord, chunk] : chunks_) {
    if (desiredChunks.find(coord) == desiredChunks.end()) {
      toDelete.push_back(coord);
    }
  }

  for (auto coord : toDelete) {
    // Find and destroy corresponding ECS entity
    auto view = registry_.view<Transform, MeshRef>();
    for (auto e : view) {
      auto &tf = view.get<Transform>(e);
      glm::vec3 entityPos = glm::vec3(tf.model[3]);
      glm::vec3 chunkWorldPos = glm::vec3(coord.x * chunkSize * voxelScale,
                                          coord.y * chunkSize * voxelScale,
                                          coord.z * chunkSize * voxelScale);

      if (glm::distance(entityPos, chunkWorldPos) < 1e-3f) {
        registry_.destroy(e);
        break;
      }
    }
    pendingDestroy_.push_back(std::move(chunks_[coord]));
    chunks_.erase(coord);
  }
}

// Spawn N threads, each with its own VkCommandPool, to pull coords from
// taskQueue_

void VulkanContext::startWorkerThreads() {
  const unsigned int N = std::max(1u, std::thread::hardware_concurrency());
  for (unsigned int i = 0; i < N; ++i) {
    workers_.emplace_back([this]() {
      // create a thread-local command pool
      VkCommandPoolCreateInfo cpci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
      cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
      cpci.queueFamilyIndex = findGraphicsQueueFamily();
      VkCommandPool threadPool;
      vkCreateCommandPool(device_, &cpci, nullptr, &threadPool);

      while (true) {
        glm::ivec3 coord;
        { // wait for work or shutdown
          std::unique_lock<std::mutex> lk(taskMux_);
          taskCV_.wait(lk,
                       [this] { return stopWorkers_ || !taskQueue_.empty(); });
          if (stopWorkers_ && taskQueue_.empty())
            break;
          coord = taskQueue_.front();
          taskQueue_.pop();
        }

        // --- CPU: generate & mesh on the CPU side ---
        std::vector<Vertex> verts;
        std::vector<uint32_t> inds;
        {
          Chunk chunk;
          chunk.chunkPos = coord;
          chunk.volume = std::make_unique<VoxelVolume>();
          const int chunkSize = 128;
          for (int x = 0; x < chunkSize; ++x)
            for (int y = 0; y < chunkSize; ++y)
              for (int z = 0; z < chunkSize; ++z) {
                int wx = coord.x * chunkSize + x;
                int wy = y;
                int wz = coord.z * chunkSize + z;
                Voxel v = generateTerrainVoxel(wx, wy, wz);
                if (v.solid)
                  chunk.volume->insert({x, y, z}, v);
              }
          chunk.volume->generateMesh(verts, inds);
        }

        // --- GPU upload on this thread, but serialize access to the single
        // queue: ---
        std::unique_ptr<Mesh> mesh;
        {
          std::lock_guard<std::mutex> ql(queueMux_);
          mesh = std::make_unique<Mesh>(device_, physicalDevice_, threadPool,
                                        graphicsQueue_, verts, inds);
        }

        // push into doneQueue_
        {
          std::lock_guard<std::mutex> lk(doneMux_);
          Chunk out;
          out.chunkPos = coord;
          out.volume = nullptr; // you already filled volume above
          out.mesh = std::move(mesh);
          out.dirty = false;
          doneQueue_.push(std::move(out));
        }
      }

      // before we destroy our threadPool, make sure all its submissions are
      // done:
      {
        std::lock_guard<std::mutex> ql(queueMux_);
        vkQueueWaitIdle(graphicsQueue_);
      }
      vkDestroyCommandPool(device_, threadPool, nullptr);
    });
  }
}

// Signal shutdown, wake all threads, join them

void VulkanContext::stopWorkerThreads() {
  // tell them to exit
  {
    std::lock_guard<std::mutex> lk(taskMux_);
    stopWorkers_ = true;
  }
  taskCV_.notify_all();

  // wait for any in-flight GPU work to finish—
  // don't destroy pools until that’s done
  vkDeviceWaitIdle(device_);

  // now join
  for (auto &t : workers_)
    t.join();
  workers_.clear();
}

void VulkanContext::initChunks() {
  const int range = 1; // how many around (0,0)
  for (int cx = -range; cx <= range; ++cx) {
    for (int cz = -range; cz <= range; ++cz) {
      glm::ivec3 coord{cx, 0, cz};
      // placeholder so updateChunks won’t re‐enqueue
      chunks_[coord] = Chunk{};
      chunks_[coord].chunkPos = coord;
      {
        std::lock_guard<std::mutex> lk(taskMux_);
        taskQueue_.push(coord);
      }
    }
  }
  taskCV_.notify_all();
}
