#pragma once
#include "engine/core/DebugUtilsMessenger.hpp"
#include <memory>
#include <vulkan/vulkan_raii.hpp>

namespace engine {

class Instance {
  public:
    Instance(vk::raii::Context &context);
    ~Instance() = default;

    const vk::raii::Instance &get() const;
    bool isValidationEnabled() const;

  private:
    vk::raii::Instance instance_;
    std::unique_ptr<DebugUtilsMessenger> debugMessenger_;
    bool validationEnabled_ = false;
};

} // namespace engine
