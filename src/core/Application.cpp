
#include "engine/core/Application.hpp"
#include "engine/world/Chunk.hpp"
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
      threadPool_(std::thread::hardware_concurrency()), uploadPool_(1),
      rendererContext_(windowManager_.getWindow()),
      inputManager_(windowManager_.getWindow(), rendererContext_.camera()),
      chunkManager_(), chunkRenderer_() {
  rendererContext_.initImGui(windowManager_.getWindow());
  chunkManager_.initChunks(threadPool_);

  // start with UI visible and cursor free
  uiMode_ = true;
  glfwSetInputMode(windowManager_.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

Application::~Application() {
  threadPool_.waitIdle();
  uploadPool_.waitIdle();
  vkDeviceWaitIdle(rendererContext_.getDevice()->getDevice());
}

void Application::Run() {
  while (!windowManager_.shouldClose())
    mainLoop();
}

void Application::mainLoop() {
  // 1) Poll events & resize
  windowManager_.pollEvents();
  if (WindowManager::framebufferResized) {
    WindowManager::framebufferResized = false;
    rendererContext_.recreateSwapchain();
  }

  // 1a) Toggle UI on Tab (edge‐detect)
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

  // 2) Delta‐time
  static double lastTime = glfwGetTime();
  double now = glfwGetTime();
  float dt = float(now - lastTime);
  lastTime = now;

  // 3) Movement & begin frame
  inputManager_.processInput(dt);
  rendererContext_.beginFrame();

  // 3a) Query‐slot for this frame
  VkCommandBuffer cmd = rendererContext_.getCurrentCommandBuffer();
  size_t frame = rendererContext_.getFrameIndex();

  // reset & begin pipeline‐stats query
  vkCmdResetQueryPool(cmd, rendererContext_.pipelineStatsQueryPool_, frame, 1);
  vkCmdBeginQuery(cmd, rendererContext_.pipelineStatsQueryPool_, frame, 0);

  // reset & begin occlusion (samples‐passed) query
  vkCmdResetQueryPool(cmd, rendererContext_.occlusionQueryPool_, frame, 1);
  vkCmdBeginQuery(cmd, rendererContext_.occlusionQueryPool_, frame, 0);

  // 4) Draw world once to gather stats
  chunkRenderer_.drawAll(rendererContext_, chunkManager_);

  // end both queries
  vkCmdEndQuery(cmd, rendererContext_.pipelineStatsQueryPool_, frame);
  vkCmdEndQuery(cmd, rendererContext_.occlusionQueryPool_, frame);

  // 5) Chunk meshing
  glm::vec3 camPos = rendererContext_.camera().getPosition();
  chunkManager_.updateChunks(camPos, threadPool_);

  // 6) Collect mesh results
  auto meshResults = threadPool_.collectResults();

  // 7) Assign new volumes
  {
    std::lock_guard<std::mutex> lock(chunkManager_.assignMtx_);
    for (auto &p : chunkManager_.chunkVolumesPending_) {
      chunkManager_.getChunk(p.first).volume = std::move(p.second);
    }
    chunkManager_.chunkVolumesPending_.clear();
  }

  // 8) Upload to GPU
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
    });
  }

  // 9) Draw world again for final pass
  chunkRenderer_.drawAll(rendererContext_, chunkManager_);

  // 10) ImGui overlay (when UI is open)
  if (uiMode_) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Debug Info");
    ImGui::Text("FPS: %.1f", 1.0f / dt);
    ImGui::Text("Camera Pos: (%.2f, %.2f, %.2f)", camPos.x, camPos.y, camPos.z);

    // show last‐frame’s stats (stored at slot = frame)
    size_t lastSlot = (frame + RendererContext::MAX_FRAMES_IN_FLIGHT - 1) %
                      RendererContext::MAX_FRAMES_IN_FLIGHT;
    ImGui::Text("Submitted tris:  %llu",
                (unsigned long long)rendererContext_.statsSubmitted_[lastSlot]);
    ImGui::Text(
        "Rasterized tris: %llu",
        (unsigned long long)rendererContext_.statsRasterized_[lastSlot]);
    ImGui::Text("Fragments drawn:  %llu",
                (unsigned long long)rendererContext_.statsSamples_[lastSlot]);
    ImGui::End();

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                    rendererContext_.getCurrentCommandBuffer());
  }

  // 11) Finish and present
  rendererContext_.endFrame();
}
