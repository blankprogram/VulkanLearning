
#include "engine/platform/VulkanDevice.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

#ifdef ENABLE_VALIDATION_LAYERS
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
  std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}
#endif

VulkanDevice::VulkanDevice(GLFWwindow *window) {
  createInstance();
#ifdef ENABLE_VALIDATION_LAYERS
  setupDebugMessenger();
#endif
  createSurface(window);
  pickPhysicalDevice();
  createLogicalDevice();
  createCommandPool();
  createAllocator();
}

VulkanDevice::~VulkanDevice() {
  if (allocator_) {
    vmaDestroyAllocator(allocator_);
  }

  if (commandPool_) {
    vkDestroyCommandPool(device_, commandPool_, nullptr);
  }

  if (device_) {
    vkDestroyDevice(device_, nullptr);
  }

  if (surface_) {
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
  }

#ifdef ENABLE_VALIDATION_LAYERS
  destroyDebugMessenger();
#endif

  if (instance_) {
    vkDestroyInstance(instance_, nullptr);
  }
}

void VulkanDevice::createInstance() {
  VkApplicationInfo appInfo{VK_STRUCTURE_TYPE_APPLICATION_INFO};
  appInfo.pApplicationName = "Vulkan Voxel Engine";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  std::vector<const char *> extensions;
  uint32_t glfwExtCount = 0;
  const char **glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
  extensions.insert(extensions.end(), glfwExts, glfwExts + glfwExtCount);

#ifdef ENABLE_VALIDATION_LAYERS
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  VkInstanceCreateInfo ci{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  ci.pApplicationInfo = &appInfo;
  ci.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  ci.ppEnabledExtensionNames = extensions.data();

#ifdef ENABLE_VALIDATION_LAYERS
  const char *validationLayer = "VK_LAYER_KHRONOS_validation";
  ci.enabledLayerCount = 1;
  ci.ppEnabledLayerNames = &validationLayer;

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  debugCreateInfo.sType =
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debugCreateInfo.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debugCreateInfo.pfnUserCallback = debugCallback;
  ci.pNext = &debugCreateInfo;
#else
  ci.enabledLayerCount = 0;
  ci.pNext = nullptr;
#endif

  if (vkCreateInstance(&ci, nullptr, &instance_) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create Vulkan instance");
  }
}

void VulkanDevice::createSurface(GLFWwindow *window) {
  if (glfwCreateWindowSurface(instance_, window, nullptr, &surface_) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create window surface");
  }
}

void VulkanDevice::pickPhysicalDevice() {
  uint32_t count = 0;
  vkEnumeratePhysicalDevices(instance_, &count, nullptr);
  if (count == 0)
    throw std::runtime_error("No GPUs with Vulkan support found!");

  std::vector<VkPhysicalDevice> devices(count);
  vkEnumeratePhysicalDevices(instance_, &count, devices.data());

  for (auto device : devices) {
    uint32_t qCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &qCount, nullptr);
    std::vector<VkQueueFamilyProperties> qProps(qCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &qCount, qProps.data());

    for (uint32_t i = 0; i < qCount; ++i) {
      VkBool32 present;
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &present);
      if ((qProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present) {
        physicalDevice_ = device;
        graphicsQueueFamilyIndex_ = i;
        return;
      }
    }
  }

  throw std::runtime_error("Failed to find suitable physical device");
}

void VulkanDevice::createLogicalDevice() {
  float priority = 1.0f;
  VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  qci.queueFamilyIndex = graphicsQueueFamilyIndex_;
  qci.queueCount = 1;
  qci.pQueuePriorities = &priority;

  const char *exts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  VkDeviceCreateInfo ci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  ci.queueCreateInfoCount = 1;
  ci.pQueueCreateInfos = &qci;
  ci.enabledExtensionCount = 1;
  ci.ppEnabledExtensionNames = exts;

  if (vkCreateDevice(physicalDevice_, &ci, nullptr, &device_) != VK_SUCCESS)
    throw std::runtime_error("Failed to create logical device");

  vkGetDeviceQueue(device_, graphicsQueueFamilyIndex_, 0, &graphicsQueue_);
}

void VulkanDevice::createCommandPool() {
  VkCommandPoolCreateInfo poolInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex_;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to create command pool");
}

void VulkanDevice::createAllocator() {
  VmaAllocatorCreateInfo allocInfo{};
  allocInfo.physicalDevice = physicalDevice_;
  allocInfo.device = device_;
  allocInfo.instance = instance_;
  if (vmaCreateAllocator(&allocInfo, &allocator_) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create VMA allocator");
  }
}

#ifdef ENABLE_VALIDATION_LAYERS
void VulkanDevice::setupDebugMessenger() {
  VkDebugUtilsMessengerCreateInfoEXT ci{};
  ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  ci.pfnUserCallback = debugCallback;

  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance_, "vkCreateDebugUtilsMessengerEXT");
  if (func && func(instance_, &ci, nullptr, &debugMessenger_) != VK_SUCCESS)
    std::cerr << "Warning: failed to install debug messenger" << std::endl;
}

void VulkanDevice::destroyDebugMessenger() {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance_, "vkDestroyDebugUtilsMessengerEXT");
  if (func && debugMessenger_) {
    func(instance_, debugMessenger_, nullptr);
    debugMessenger_ = VK_NULL_HANDLE;
  }
}
#endif
