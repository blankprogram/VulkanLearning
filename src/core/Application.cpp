#include "engine/core/Application.hpp"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
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

Application::~Application() {
  // Make sure the GPU is idle, then clean up
  vkDeviceWaitIdle(rendererContext_.getDevice()->getDevice());
  rendererContext_.cleanup();
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

  glm::vec3 camPos = rendererContext_.camera().getPosition();

  // enqueue any new mesh jobs
  chunkManager_.updateChunks(camPos, threadPool_);

  // collect finished meshes
  auto results = threadPool_.collectResults();

  for (auto &r : results) {
    try {
      Chunk &chunk = chunkManager_.getChunk(r.coord);

      if (!chunk.mesh) {
        chunk.mesh = std::move(r.mesh);
        chunk.mesh->uploadToGPU(rendererContext_.getDevice());

        chunk.dirty = false;
        chunk.meshJobQueued = false;
        std::cout << "[MainLoop] Uploaded mesh for " << r.coord.x << ", "
                  << r.coord.y << ", " << r.coord.z << "\n";
      }
    } catch (const std::out_of_range &) {
      std::cerr << "[MainLoop] Skipped mesh upload: chunk " << r.coord.x << ", "
                << r.coord.y << ", " << r.coord.z << " not found\n";
    }
  }

  // render all chunks
  chunkRenderer_.drawAll(rendererContext_, chunkManager_);

  rendererContext_.endFrame();
}
