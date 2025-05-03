#pragma once

namespace engine {

struct DeviceConfig {
  bool enableAnisotropy = false;
  bool enableWideLines = false;
};
inline const DeviceConfig deviceConfig{};
} // namespace engine
