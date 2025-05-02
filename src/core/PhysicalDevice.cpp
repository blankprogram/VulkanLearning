#include "engine/core/PhysicalDevice.hpp"
#include <stdexcept>

namespace engine {

PhysicalDevice::PhysicalDevice(const vk::raii::Instance &instance)
    : device_([&]() -> vk::raii::PhysicalDevice {
          vk::raii::PhysicalDevices physicalDevices(instance);
          if (physicalDevices.empty()) {
              throw std::runtime_error("No physical devices found");
          }
          return std::move(physicalDevices.front());
      }()) {}

const vk::raii::PhysicalDevice &PhysicalDevice::get() const { return device_; }

vk::PhysicalDeviceProperties PhysicalDevice::getProperties() const {
    return device_.getProperties();
}

vk::PhysicalDeviceMemoryProperties PhysicalDevice::getMemoryProperties() const {
    return device_.getMemoryProperties();
}

} // namespace engine
