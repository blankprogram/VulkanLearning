#pragma once

#include "engine/platform/InputManager.hpp"
#include "engine/platform/RendererContext.hpp"
#include "engine/platform/WindowManager.hpp"
#include "engine/utils/ThreadPool.hpp"
#include "engine/world/ChunkManager.hpp"
#include "engine/world/ChunkRenderSystem.hpp"

class Application {
  public:
    Application();
    ~Application();
    void Run();

  private:
    void mainLoop();

    WindowManager windowManager_;
    engine::utils::ThreadPool threadPool_;
    engine::world::ChunkManager chunkManager_;
    engine::world::ChunkRenderSystem chunkRenderer_;
    RendererContext rendererContext_;
    InputManager inputManager_;
    engine::utils::ThreadPool uploadPool_;
};
