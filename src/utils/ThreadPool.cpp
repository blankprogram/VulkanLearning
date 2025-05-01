#include "engine/utils/ThreadPool.hpp"

using namespace engine::utils;

ThreadPool::ThreadPool(size_t count) {
  for (size_t i = 0; i < count; ++i) {
    workers_.emplace_back([this] {
      while (true) {
        std::function<void()> job;
        { // pop job
          std::unique_lock lk(jobsMtx_);
          jobsCv_.wait(lk, [this] { return stop_ || !jobs_.empty(); });
          if (stop_ && jobs_.empty())
            return;
          // grab one job
          job = std::move(jobs_.front());
          jobs_.pop();
          // note: this task has now “moved” from queued→running
        }
        // run it
        job();
        // one job done
        {
          std::lock_guard lk(jobsMtx_);
          --tasksInFlight_;
        }
        idleCv_.notify_one();
      }
    });
  }
}

ThreadPool::~ThreadPool() {
  // wait for all pending/running tasks to finish
  waitIdle();

  // signal shutdown
  {
    std::lock_guard lk(jobsMtx_);
    stop_ = true;
  }
  jobsCv_.notify_all();

  // join threads
  for (auto &w : workers_)
    w.join();
}

void ThreadPool::enqueueJob(std::function<void()> job) {
  {
    std::lock_guard lk(jobsMtx_);
    jobs_.push(std::move(job));
    ++tasksInFlight_;
  }
  jobsCv_.notify_one();
}

void ThreadPool::enqueueMesh(const glm::ivec3 &coord,
                             std::function<std::unique_ptr<Mesh>()> func) {
  enqueueJob([this, coord, func = std::move(func)]() mutable {
    auto mesh = func();
    {
      std::lock_guard lk(resultsMtx_);
      results_.push({coord, std::move(mesh)});
    }
  });
}

std::vector<MeshResult> ThreadPool::collectResults() {
  std::vector<MeshResult> out;
  std::lock_guard lk(resultsMtx_);
  while (!results_.empty()) {
    out.push_back(std::move(results_.front()));
    results_.pop();
  }
  return out;
}

void ThreadPool::waitIdle() {
  std::unique_lock lk(jobsMtx_);
  idleCv_.wait(lk, [this] { return tasksInFlight_ == 0; });
}
