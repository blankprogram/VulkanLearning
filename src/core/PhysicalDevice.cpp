#include "engine/core/PhysicalDevice.hpp"
#include <stdexcept>

namespace engine {

static vk::raii::PhysicalDevice
pickPhysicalDevice(const vk::raii::Instance &instance) {
    vk::raii::PhysicalDevices devices(instance);
    if (devices.empty()) {
        throw std::runtime_error("No physical devices found");
    }
    return std::move(devices.front());
}

PhysicalDevice::PhysicalDevice(const vk::raii::Instance &instance)
    : device_(pickPhysicalDevice(instance)) {}

const vk::raii::PhysicalDevice &PhysicalDevice::get() const { return device_; }

vk::PhysicalDeviceProperties PhysicalDevice::getProperties() const {
    return device_.getProperties();
}

vk::PhysicalDeviceMemoryProperties PhysicalDevice::getMemoryProperties() const {
    return device_.getMemoryProperties();
}

} // namespace engine
