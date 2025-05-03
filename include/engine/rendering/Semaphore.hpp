#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace engine {

class Semaphore {
public:
  explicit Semaphore(const vk::raii::Device &device);
  ~Semaphore() = default;
  Semaphore(const Semaphore &) = delete;
  Semaphore &operator=(const Semaphore &) = delete;
  Semaphore(Semaphore &&) = default;
  Semaphore &operator=(Semaphore &&) = default;

  VkSemaphore get() const { return *sem_; }

private:
  vk::raii::Semaphore sem_;
};

} // namespace engine
