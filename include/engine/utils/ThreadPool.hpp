
// include/engine/utils/ThreadPool.hpp
#pragma once

#include "engine/render/Mesh.hpp"
#include <condition_variable>
#include <functional>
#include <glm/vec3.hpp>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace engine::utils {

struct MeshResult {
  glm::ivec3 coord;
  std::unique_ptr<Mesh> mesh;
};

class ThreadPool {
public:
  ThreadPool(size_t workerCount = std::thread::hardware_concurrency());
  ~ThreadPool();

  // enqueue any job
  void enqueueJob(std::function<void()> job);

  // enqueue a meshing job: generate mesh & push result
  void enqueueMesh(const glm::ivec3 &coord,
                   std::function<std::unique_ptr<Mesh>()> func);

  // call on main thread to collect completed MeshResults
  std::vector<MeshResult> collectResults();

  // block until all queued+running jobs are complete
  void waitIdle();

private:
  // worker threads
  std::vector<std::thread> workers_;

  // pending jobs
  std::queue<std::function<void()>> jobs_;
  std::mutex jobsMtx_;
  std::condition_variable jobsCv_;

  // results
  std::queue<MeshResult> results_;
  std::mutex resultsMtx_;

  // shutdown flag
  bool stop_ = false;

  // for waitIdle(): count of queued+running tasks
  size_t tasksInFlight_ = 0;
  std::condition_variable idleCv_;
};

} // namespace engine::utils
