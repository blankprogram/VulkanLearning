#pragma once

#include <optional>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class Queue {
public:
  struct FamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
    std::optional<uint32_t> compute;
    std::optional<uint32_t> transfer;

    bool isComplete() const;
  };

  static FamilyIndices
  findQueueFamilies(const vk::raii::PhysicalDevice &physical,
                    const vk::raii::SurfaceKHR &surface);
};

} // namespace engine
