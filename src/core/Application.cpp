#include "engine/core/Application.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

using namespace engine;
using namespace engine::world;

Application::Application()
    : windowManager_(1280, 720, "Vulkan Voxel World"),
      threadPool_(std::thread::hardware_concurrency()),
      rendererContext_(windowManager_.getWindow()),
      inputManager_(windowManager_.getWindow(), rendererContext_.camera()),
      chunkManager_(), chunkRenderer_() {
  // initial chunk load
  chunkManager_.initChunks(threadPool_);
}

void Application::Run() {
  while (!windowManager_.shouldClose())
    mainLoop();
}

void Application::mainLoop() {
  windowManager_.pollEvents();
  if (WindowManager::framebufferResized) {
    WindowManager::framebufferResized = false;
    rendererContext_.recreateSwapchain();
  }
  static double last = glfwGetTime();
  double now = glfwGetTime();
  float dt = float(now - last);
  last = now;

  inputManager_.processInput(dt);
  rendererContext_.beginFrame();

  // get camera position (stubbed)
  glm::vec3 camPos{0.0f, 0.0f, 0.0f}; // TODO: replace with actual camera query

  // enqueue any new mesh jobs
  chunkManager_.updateChunks(camPos, threadPool_);

  // collect finished meshes
  auto results = threadPool_.collectResults();
  for (auto &r : results) {
    auto &chunk = chunkManager_.getChunk(r.coord);
    chunk.mesh = std::move(r.mesh);
    chunk.mesh->uploadToGPU(rendererContext_.getDevice());
    chunk.dirty = false;
    chunk.meshJobQueued = false;
  }

  // render all chunks
  chunkRenderer_.drawAll(rendererContext_, chunkManager_);

  rendererContext_.endFrame();
}
