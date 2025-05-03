
#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace engine {

class Context {
public:
  Context();
  ~Context() = default;

  vk::raii::Context &get();

private:
  vk::raii::Context context_;
};

} // namespace engine
