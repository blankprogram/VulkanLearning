#include "include/engine/platform/VulkanContext.hpp"
#include <iostream>
int main() {
  try {
    VulkanContext app{800, 600, "Vulkan Triangle"};
    app.Run();
  } catch (const std::exception &e) {
    std::cerr << "Fatal error: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
