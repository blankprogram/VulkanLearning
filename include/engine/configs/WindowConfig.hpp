#pragma once

namespace engine {

struct WindowConfig {
  int width = 720;
  int height = 1280;
  const char *name = "VoxelLand"; // Like lego land ykykyk
};

inline const WindowConfig windowConfig{};

} // namespace engine
