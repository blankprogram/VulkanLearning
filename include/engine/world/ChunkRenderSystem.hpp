
// include/engine/world/ChunkRenderSystem.hpp
#pragma once

#include "engine/platform/RendererContext.hpp"
#include "engine/world/ChunkManager.hpp"

namespace engine::world {

class ChunkRenderSystem {
public:
  void drawAll(RendererContext &ctx, const ChunkManager &chunks);
};

} // namespace engine::world
