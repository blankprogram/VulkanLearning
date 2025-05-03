#pragma once

#include "engine/core/Queue.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class Device {
public:
  Device(const vk::raii::PhysicalDevice &physical,
         const vk::raii::SurfaceKHR &surface);

  ~Device() = default;
  Device(const Device &) = delete;
  Device &operator=(const Device &) = delete;
  Device(Device &&) = default;
  Device &operator=(Device &&) = default;

  const vk::raii::Device &get() const { return device_; }

  const vk::raii::Queue &graphicsQueue() const { return graphicsQueue_; }
  const vk::raii::Queue &presentQueue() const { return presentQueue_; }
  const vk::raii::Queue &computeQueue() const { return computeQueue_; }
  const vk::raii::Queue &transferQueue() const { return transferQueue_; }

private:
  Queue::FamilyIndices indices_;
  vk::raii::Device device_;
  vk::raii::Queue graphicsQueue_;
  vk::raii::Queue presentQueue_;
  vk::raii::Queue computeQueue_;
  vk::raii::Queue transferQueue_;
};

} // namespace engine
