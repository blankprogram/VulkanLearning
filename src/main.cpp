
#include "engine/platform/WindowManager.hpp"
#include <GLFW/glfw3.h>

int main() {
  WindowManager windowManager(1280, 720, "Hello Vulkan");
  GLFWwindow *window = windowManager.getWindow();

  // Main loop
  while (!windowManager.shouldClose()) {
    glfwPollEvents();
    glfwSwapBuffers(window);
    // drawFrame(); (future)
  }

  return 0;
}
