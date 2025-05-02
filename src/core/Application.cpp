#include "engine/core/Application.hpp"
#include "engine/world/Chunk.hpp"
#include <engine/render/Camera.hpp>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

using namespace engine;
using namespace engine::world;

Application::Application()
    : windowManager_(1280, 720, "Vulkan Voxel World"),
      threadPool_(std::thread::hardware_concurrency()), chunkManager_(),
      chunkRenderer_(), rendererContext_(windowManager_.getWindow()),
      inputManager_(windowManager_.getWindow(), rendererContext_.camera()),
      uploadPool_(1) {
    rendererContext_.initImGui(windowManager_.getWindow());
    chunkManager_.initChunks(threadPool_);
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
    windowManager_.pollEvents();
    if (WindowManager::framebufferResized) {
        WindowManager::framebufferResized = false;
        rendererContext_.recreateSwapchain();
    }

    static double lastTime = glfwGetTime();
    double now = glfwGetTime();
    float dt = float(now - lastTime);
    lastTime = now;
    inputManager_.processInput(dt);
    rendererContext_.beginFrame();
    VkCommandBuffer cmd = rendererContext_.getCurrentCommandBuffer();
    size_t frame = rendererContext_.getFrameIndex();

    vkCmdResetQueryPool(cmd, rendererContext_.pipelineStatsQueryPool_, frame,
                        1);
    vkCmdBeginQuery(cmd, rendererContext_.pipelineStatsQueryPool_, frame, 0);

    vkCmdResetQueryPool(cmd, rendererContext_.occlusionQueryPool_, frame, 1);
    vkCmdBeginQuery(cmd, rendererContext_.occlusionQueryPool_, frame, 0);

    chunkRenderer_.drawAll(rendererContext_, chunkManager_);

    vkCmdEndQuery(cmd, rendererContext_.pipelineStatsQueryPool_, frame);
    vkCmdEndQuery(cmd, rendererContext_.occlusionQueryPool_, frame);

    glm::vec3 camPos = rendererContext_.camera().getPosition();
    chunkManager_.updateChunks(camPos, threadPool_);

    auto meshResults = threadPool_.collectResults();
    {
        std::lock_guard<std::mutex> lock(chunkManager_.assignMtx_);
        for (auto &p : chunkManager_.chunkVolumesPending_) {
            chunkManager_.getChunk(p.first).volume = std::move(p.second);
        }
        chunkManager_.chunkVolumesPending_.clear();
    }

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

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Debug Info");
    ImGui::Text("FPS: %.1f", 1.0f / dt);
    ImGui::Text("Camera Pos: (%.2f, %.2f, %.2f)", camPos.x, camPos.y, camPos.z);

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
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    rendererContext_.endFrame();
}
