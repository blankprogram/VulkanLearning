#pragma once
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class DebugUtilsMessenger {
  public:
    DebugUtilsMessenger(const vk::raii::Instance &instance);
    ~DebugUtilsMessenger() = default;

  private:
    vk::raii::DebugUtilsMessengerEXT messenger_;
};

} // namespace engine
