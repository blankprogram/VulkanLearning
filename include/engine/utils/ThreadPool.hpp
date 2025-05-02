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

    void enqueueJob(std::function<void()> job);

    void enqueueMesh(const glm::ivec3 &coord,
                     std::function<std::unique_ptr<Mesh>()> func);

    std::vector<MeshResult> collectResults();

    void waitIdle();

  private:
    std::vector<std::thread> workers_;

    std::queue<std::function<void()>> jobs_;
    std::mutex jobsMtx_;
    std::condition_variable jobsCv_;

    std::queue<MeshResult> results_;
    std::mutex resultsMtx_;

    bool stop_ = false;

    size_t tasksInFlight_ = 0;
    std::condition_variable idleCv_;
};

} // namespace engine::utils
