#pragma once

#include "engine/platform/RendererContext.hpp"
#include "engine/world/ChunkManager.hpp"

namespace engine::world {

class ChunkRenderSystem {
public:
  void drawAll(RendererContext &ctx, ChunkManager &chunks);
};

} // namespace engine::world
