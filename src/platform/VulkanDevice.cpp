
#include "engine/platform/VulkanDevice.hpp"
#include <cstring>
#include <set>

VulkanDevice::VulkanDevice(GLFWwindow *window) {
  createInstance();

  // Create surface using GLFW
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create window surface!");
  }

  pickPhysicalDevice();
  createLogicalDevice();
}

VulkanDevice::~VulkanDevice() {
  if (commandPool != VK_NULL_HANDLE) {
    vkDestroyCommandPool(device, commandPool, nullptr);
  }
  if (device != VK_NULL_HANDLE) {
    vkDestroyDevice(device, nullptr);
  }
  if (surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(instance, surface, nullptr);
  }
  if (instance != VK_NULL_HANDLE) {
    vkDestroyInstance(instance, nullptr);
  }
}

void VulkanDevice::createInstance() {
  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Vulkan Engine";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;

  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = glfwExtensionCount;
  createInfo.ppEnabledExtensionNames = glfwExtensions;

  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create Vulkan instance!");
  }
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
