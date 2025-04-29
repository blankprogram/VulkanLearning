
#pragma once
#include <vector>
#include <vulkan/vulkan.h>

class FrameSync {
public:
  void init(VkDevice device, size_t framesInFlight);
  void cleanup(VkDevice device);

  VkSemaphore getImageAvailable(size_t frame) const {
    return imageAvailableSemaphores_[frame];
  }
  VkSemaphore getRenderFinished(size_t frame) const {
    return renderFinishedSemaphores_[frame];
  }
  VkFence getInFlightFence(size_t frame) const {
    return inFlightFences_[frame];
  }

private:
  std::vector<VkSemaphore> imageAvailableSemaphores_;
  std::vector<VkSemaphore> renderFinishedSemaphores_;
  std::vector<VkFence> inFlightFences_;
};
