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

private:
  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> jobs_;
  std::mutex jobsMtx_;
  std::condition_variable jobsCv_;
  bool stop_ = false;

  std::queue<MeshResult> results_;
  std::mutex resultsMtx_;
};

} // namespace engine::utils
