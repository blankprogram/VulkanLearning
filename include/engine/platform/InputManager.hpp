
#include "engine/ecs/ECSManager.hpp"
#include "engine/render/Camera.hpp"
#include <GLFW/glfw3.h>

class InputManager {
public:
  InputManager(GLFWwindow *window);
  void processInput(float dt);
  void onMouseMoved(float xpos, float ypos);

private:
  GLFWwindow *window_;
  float lastX_, lastY_;
  bool firstMouse_;
};
