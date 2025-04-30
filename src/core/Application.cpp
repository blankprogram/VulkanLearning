
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

  glm::vec3 camPos = rendererContext_.camera().getPosition();

  chunkManager_.updateChunks(camPos, threadPool_);

  auto results = threadPool_.collectResults();
  for (auto &r : results) {
    try {
      Chunk &chunk = chunkManager_.getChunk(glm::ivec2(r.coord.x, r.coord.z));
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

  chunkRenderer_.drawAll(rendererContext_, chunkManager_);

  // ImGui debug window
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

  rendererContext_.endFrame();
}
