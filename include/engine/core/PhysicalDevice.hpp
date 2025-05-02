#pragma once
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class PhysicalDevice {
  public:
    PhysicalDevice(const vk::raii::Instance &instance);
    ~PhysicalDevice() = default;

    const vk::raii::PhysicalDevice &get() const;
    vk::PhysicalDeviceProperties getProperties() const;
    vk::PhysicalDeviceMemoryProperties getMemoryProperties() const;

  private:
    vk::raii::PhysicalDevice device_;
};

} // namespace engine
