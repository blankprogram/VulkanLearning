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
      uploadPool_(1), // ONE thread for copy+fence waits
      rendererContext_(windowManager_.getWindow()),
      inputManager_(windowManager_.getWindow(), rendererContext_.camera()),
      chunkManager_(), chunkRenderer_() {
  rendererContext_.initImGui(windowManager_.getWindow());
  chunkManager_.initChunks(threadPool_);

  // Start with UI open and cursor free
  uiMode_ = true;
  glfwSetInputMode(windowManager_.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

Application::~Application() {
  threadPool_.waitIdle();
  uploadPool_.waitIdle();
  vkDeviceWaitIdle(rendererContext_.getDevice()->getDevice());
  // RendererContext destructor will clean up Vulkan
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

  // 1a) Toggle UI with Tab (edge‐detect)
  if (glfwGetKey(windowManager_.getWindow(), GLFW_KEY_TAB) == GLFW_PRESS &&
      !tabPressed_) {
    tabPressed_ = true;
    uiMode_ = !uiMode_;
    glfwSetInputMode(windowManager_.getWindow(), GLFW_CURSOR,
                     uiMode_ ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
  }
  if (glfwGetKey(windowManager_.getWindow(), GLFW_KEY_TAB) == GLFW_RELEASE) {
    tabPressed_ = false;
  }

  // 2) Compute delta‐time
  static double last = glfwGetTime();
  double now = glfwGetTime();
  float dt = float(now - last);
  last = now;

  // 3) Input (only when UI is off) & begin frame
  inputManager_.processInput(dt);
  rendererContext_.beginFrame();

  // 4) Kick off any new chunk‐meshing work
  glm::vec3 camPos = rendererContext_.camera().getPosition();
  chunkManager_.updateChunks(camPos, threadPool_);

  // 5) Pull back meshes that threads have finished building
  auto meshResults = threadPool_.collectResults();

  // 6) Assign the newly generated volumes on the main thread
  {
    std::lock_guard<std::mutex> lock(chunkManager_.assignMtx_);
    for (auto &p : chunkManager_.chunkVolumesPending_) {
      chunkManager_.getChunk(p.first).volume = std::move(p.second);
    }
    chunkManager_.chunkVolumesPending_.clear();
  }

  // 7) Hand off each mesh to uploadPool_ (so we don't stall the main thread)
  for (auto &r : meshResults) {
    glm::ivec2 coord2{r.coord.x, r.coord.z};
    std::unique_ptr<Mesh> meshPtr = std::move(r.mesh);
    Mesh *rawMesh = meshPtr.release();

    uploadPool_.enqueueJob([this, coord2, rawMesh]() {
      std::unique_ptr<Mesh> meshUp(rawMesh);
      meshUp->uploadToGPU(rendererContext_.getDevice());

      std::lock_guard<std::mutex> lock(chunkManager_.assignMtx_);
      auto &chunk = chunkManager_.getChunk(coord2);
      chunk.mesh = std::move(meshUp);
      chunk.dirty = false;
      chunk.meshJobQueued = false;
      std::cout << "[UploadThread] Uploaded mesh for " << coord2.x << ", "
                << coord2.y << "\n";
    });
  }

  // 8) Draw world
  chunkRenderer_.drawAll(rendererContext_, chunkManager_);

  // 9) ImGui overlay (only when UI is on)
  if (uiMode_) {
    // compute some debug info
    size_t totalTris = 0;
    for (auto &c : chunkManager_.getChunks()) {
      if (c.second.mesh)
        totalTris += c.second.mesh->indexCount() / 3;
    }

    ImGui::SetNextWindowSize(ImVec2(700, 300), ImGuiCond_Always);
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("Debug Info");
    ImGui::Text("FPS: %.1f", 1.0f / dt);
    ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", camPos.x, camPos.y,
                camPos.z);
    ImGui::Text("Triangles: %zu", totalTris);
    ImGui::End();
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                    rendererContext_.getCurrentCommandBuffer());
  }

  // 10) Finish frame
  rendererContext_.endFrame();
}
