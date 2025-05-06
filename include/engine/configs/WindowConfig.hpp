#pragma once

namespace engine {

struct WindowConfig {
  int width = 1280;
  int height = 720;
  const char *name = "VoxelLand"; // Like lego land ykykyk
};

inline const WindowConfig windowConfig{};

} // namespace engine
