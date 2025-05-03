#include "engine/app/Application.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vulkan/vulkan_raii.hpp>

int main() {
  try {
    engine::Application app;
    app.run();
  } catch (const std::exception &e) {
    std::cerr << "Fatal: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
