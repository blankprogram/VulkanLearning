
#include "engine/core/Application.hpp"
#include "engine/world/Chunk.hpp" // For Chunk struct
#include <engine/render/Camera.hpp>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>

using namespace engine;
using namespace engine::world;

Application::Application()
    : windowManager_(1280, 720, "Vulkan Voxel World"),
      threadPool_(std::thread::hardware_concurrency()),
      uploadPool_(1), // ← ONE thread for the copy+fence waits
      rendererContext_(windowManager_.getWindow()),
      inputManager_(windowManager_.getWindow(), rendererContext_.camera()),
      chunkManager_(), chunkRenderer_() {
  rendererContext_.initImGui(windowManager_.getWindow());
  chunkManager_.initChunks(threadPool_);
}

Application::~Application() {
  vkDeviceWaitIdle(rendererContext_.getDevice()->getDevice());
  rendererContext_.cleanup();
}

void Application::Run() {
  while (!windowManager_.shouldClose())
    mainLoop();
}

void Application::mainLoop() {
  // 1) Poll events & handle resize
  windowManager_.pollEvents();
  if (WindowManager::framebufferResized) {
    WindowManager::framebufferResized = false;
    rendererContext_.recreateSwapchain();
  }

  // 2) Compute delta‐time
  static double last = glfwGetTime();
  double now = glfwGetTime();
  float dt = float(now - last);
  last = now;

  // 3) Input & begin frame
  inputManager_.processInput(dt);
  rendererContext_.beginFrame();

  // 4) Kick off any new chunk‐meshing work
  glm::vec3 camPos = rendererContext_.camera().getPosition();
  chunkManager_.updateChunks(camPos, threadPool_);

  // 5) Pull back meshes that threads have finished building
  auto meshResults = threadPool_.collectResults();

  // 6) Assign the newly generated volumes (still on main thread)
  {
    std::lock_guard<std::mutex> lock(chunkManager_.assignMtx_);
    for (auto &[coord, volume] : chunkManager_.chunkVolumesPending_) {
      chunkManager_.getChunk(coord).volume = std::move(volume);
    }
    chunkManager_.chunkVolumesPending_.clear();
  }

  // 7) Hand off each mesh to uploadPool_ (so we don't stall the main thread)
  for (auto &r : meshResults) {
    glm::ivec2 coord2{r.coord.x, r.coord.z};
    // take ownership of the unique_ptr, then release to a raw pointer
    std::unique_ptr<Mesh> meshPtr = std::move(r.mesh);
    Mesh *rawMesh = meshPtr.release();

    uploadPool_.enqueueJob([this, coord2, rawMesh]() {
      // this runs on the single upload thread:
      std::unique_ptr<Mesh> meshUp(rawMesh);
      meshUp->uploadToGPU(rendererContext_.getDevice());

      // now stick it back into the chunk under the lock
      std::lock_guard<std::mutex> lock(chunkManager_.assignMtx_);
      auto &chunk = chunkManager_.getChunk(coord2);
      chunk.mesh = std::move(meshUp);
      chunk.dirty = false;
      chunk.meshJobQueued = false;
      std::cout << "[UploadThread] Uploaded mesh for " << coord2.x << ", "
                << coord2.y << "\n";
    });
  }

  // 8) Draw everything
  chunkRenderer_.drawAll(rendererContext_, chunkManager_);

  // 9) ImGui overlay
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::Begin("Debug Info");
  ImGui::Text("FPS: %.1f", 1.0f / dt);
  ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", camPos.x, camPos.y,
              camPos.z);
  ImGui::End();
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                  rendererContext_.getCurrentCommandBuffer());

  // 10) Finish frame
  rendererContext_.endFrame();
}
