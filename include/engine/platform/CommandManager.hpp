
#pragma once
#include <vulkan/vulkan.h>

class CommandManager {
public:
  void init(VkDevice device, uint32_t queueFamilyIndex);
  void cleanup(VkDevice device);

  VkCommandPool getCommandPool() const { return commandPool_; }

private:
  VkCommandPool commandPool_ = VK_NULL_HANDLE;
};
