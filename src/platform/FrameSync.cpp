
#include "engine/platform/FrameSync.hpp"
#include <stdexcept>

void FrameSync::init(VkDevice device, size_t framesInFlight) {
  imageAvailableSemaphores_.resize(framesInFlight);
  renderFinishedSemaphores_.resize(framesInFlight);
  inFlightFences_.resize(framesInFlight);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled to avoid
                                                  // waiting on first frame

  for (size_t i = 0; i < framesInFlight; ++i) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &renderFinishedSemaphores_[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences_[i]) !=
            VK_SUCCESS) {
      throw std::runtime_error(
          "Failed to create frame synchronization objects!");
    }
  }
}

void FrameSync::cleanup(VkDevice device) {
  for (size_t i = 0; i < inFlightFences_.size(); ++i) {
    if (inFlightFences_[i]) {
      vkDestroyFence(device, inFlightFences_[i], nullptr);
    }
    if (renderFinishedSemaphores_[i]) {
      vkDestroySemaphore(device, renderFinishedSemaphores_[i], nullptr);
    }
    if (imageAvailableSemaphores_[i]) {
      vkDestroySemaphore(device, imageAvailableSemaphores_[i], nullptr);
    }
  }

  inFlightFences_.clear();
  renderFinishedSemaphores_.clear();
  imageAvailableSemaphores_.clear();
}
