#include "engine/platform/VulkanDevice.hpp"
#include "engine/utils/VulkanHelpers.hpp"
#include "engine/voxel/VoxelVolume.hpp"
#include "engine/world/Chunk.hpp"
#include <condition_variable>
#include <glm/glm.hpp>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <vulkan/vulkan.h>
class ThreadPool {
public:
  ThreadPool(VulkanDevice &device, VkQueue queue);
  ~ThreadPool();
  void enqueue(const glm::ivec3 &coord);

private:
  std::vector<std::thread> workers_;
  std::queue<glm::ivec3> taskQueue_;
  // mutex, condition variable...
};
