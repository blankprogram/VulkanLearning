
#include "engine/platform/VulkanDevice.hpp"
#include <cstring>
#include <set>
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageTypes,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void * /*pUserData*/) {
  // You can filter on severity / type here if you like:
  std::cerr << "Validation layer: " << pCallbackData->pMessage << "\n";
  // Return VK_FALSE to indicate that the call should not be aborted
  return VK_FALSE;
}

static void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
  createInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;
  createInfo.pUserData = nullptr; // optional
}

// ————————————————————————————————————————————————————————————————
//  3) Load & wrap vkCreateDebugUtilsMessengerEXT
// ————————————————————————————————————————————————————————————————
static VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func) {
    return func(instance, pCreateInfo, pAllocator, pMessenger);
  }
  return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// ————————————————————————————————————————————————————————————————
//  4) Load & wrap vkDestroyDebugUtilsMessengerEXT
// ————————————————————————————————————————————————————————————————
static void
DestroyDebugUtilsMessengerEXT(VkInstance instance,
                              VkDebugUtilsMessengerEXT messenger,
                              const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func) {
    func(instance, messenger, pAllocator);
  }
}

// ————————————————————————————————————————————————————————————————
//  5) User‐facing setup/teardown
// ————————————————————————————————————————————————————————————————
VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

void setupDebugMessenger(VkInstance instance) {
  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateDebugMessengerCreateInfo(createInfo);
  if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                   &debugMessenger) != VK_SUCCESS) {
    std::cerr << "ERROR: failed to set up debug messenger!\n";
  }
}

void teardownDebugMessenger(VkInstance instance) {
  if (debugMessenger != VK_NULL_HANDLE) {
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
  }
}
VulkanDevice::VulkanDevice(GLFWwindow *window) {
  createInstance();

  // Create surface using GLFW
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create window surface!");
  }

  pickPhysicalDevice();
  createLogicalDevice();

  VmaAllocatorCreateInfo allocInfo{};
  allocInfo.physicalDevice = physicalDevice;
  allocInfo.device = device;
  allocInfo.instance = instance;
  // If you’re on multiple queues / transfer queues, set those here:
  vmaCreateAllocator(&allocInfo, &allocator);
}

void VulkanDevice::createInstance() {
  VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.pApplicationName = "Vulkan Engine";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  // 1) figure out layers + extensions
  std::vector<const char *> layers;
  std::vector<const char *> exts;

#ifdef ENABLE_VALIDATION_LAYERS
  layers.push_back("VK_LAYER_KHRONOS_validation");
  exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
  // GLFW wants its own surface extensions, too…
  uint32_t gefCount = 0;
  auto glfwExts = glfwGetRequiredInstanceExtensions(&gefCount);
  exts.insert(exts.end(), glfwExts, glfwExts + gefCount);

  // 2) build the InstanceCreateInfo
  VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  ci.pApplicationInfo = &appInfo;
  ci.enabledLayerCount = (uint32_t)layers.size();
  ci.ppEnabledLayerNames = layers.data();
  ci.enabledExtensionCount = (uint32_t)exts.size();
  ci.ppEnabledExtensionNames = exts.data();

#ifdef ENABLE_VALIDATION_LAYERS
  // chain in our debug‐utils create info so messenger is alive immediately
  VkDebugUtilsMessengerCreateInfoEXT debugCI;
  populateDebugMessengerCreateInfo(debugCI);
  ci.pNext = &debugCI;
#else
  ci.pNext = nullptr;
#endif

  if (vkCreateInstance(&ci, nullptr, &instance) != VK_SUCCESS)
    throw std::runtime_error("Failed to create Vulkan instance!");

#ifdef ENABLE_VALIDATION_LAYERS
  // if you didn’t pNext it in above, you can do it now:
  if (CreateDebugUtilsMessengerEXT(instance, &debugCI, nullptr,
                                   &debugMessenger) != VK_SUCCESS)
    std::cerr << "Warning: failed to install debug messenger\n";
#endif
}

VulkanDevice::~VulkanDevice() {
#ifdef ENABLE_VALIDATION_LAYERS
  teardownDebugMessenger(instance);
#endif
  if (commandPool != VK_NULL_HANDLE)
    vkDestroyCommandPool(device, commandPool, nullptr);
  if (device != VK_NULL_HANDLE)
    vkDestroyDevice(device, nullptr);
  if (surface != VK_NULL_HANDLE)
    vkDestroySurfaceKHR(instance, surface, nullptr);
  if (instance != VK_NULL_HANDLE)
    vkDestroyInstance(instance, nullptr);
}
void VulkanDevice::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  if (deviceCount == 0) {
    throw std::runtime_error("No GPUs with Vulkan support found!");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for (const auto &dev : devices) {
    // Find a device with graphics + presentation support
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount,
                                             queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; ++i) {
      VkBool32 supportsPresent = false;
      vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &supportsPresent);

      if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
          supportsPresent) {
        physicalDevice = dev;
        graphicsQueueFamilyIndex = i;
        return;
      }
    }
  }

  throw std::runtime_error(
      "Failed to find a suitable GPU with graphics and present support!");
}

void VulkanDevice::createLogicalDevice() {
  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  // — the swapchain extension is *mandatory* if you want to create a swapchain
  const std::vector<const char *> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  VkPhysicalDeviceFeatures deviceFeatures{}; // as before

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.queueCreateInfoCount = 1;
  createInfo.pEnabledFeatures = &deviceFeatures;

  // <-- enable swapchain here
  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create logical device!");
  }

  vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);

  // Create command pool
  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create command pool!");
  }
}
