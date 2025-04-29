#pragma once

#include "engine/platform/RendererContext.hpp"
#include "engine/platform/WindowManager.hpp"
#include "engine/utils/ThreadPool.hpp"
#include "engine/world/ChunkManager.hpp"
#include "engine/world/ChunkRenderSystem.hpp"

class Application {
public:
  Application();
  void Run();

private:
  void mainLoop();

  WindowManager windowManager_;                    // global-scope class
  engine::utils::ThreadPool threadPool_;           // fully qualified
  RendererContext rendererContext_;                // global-scope class
  engine::world::ChunkManager chunkManager_;       // fully qualified
  engine::world::ChunkRenderSystem chunkRenderer_; // fully qualified
};
