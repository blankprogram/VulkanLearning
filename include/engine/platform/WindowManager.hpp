#include <GLFW/glfw3.h>
#include <stdexcept>
#include <string>

class WindowManager {
  public:
    WindowManager(uint32_t width, uint32_t height, const std::string &title);
    ~WindowManager();

    WindowManager(const WindowManager &) = delete;
    WindowManager(WindowManager &&) = delete;
    WindowManager &operator=(const WindowManager &) = delete;
    WindowManager &operator=(WindowManager &&) = delete;

    GLFWwindow *getWindow();
    bool shouldClose();
    void pollEvents();

    static bool framebufferResized;

  private:
    GLFWwindow *window_;
    void setupCallbacks();

    static void framebufferResizeCallback(GLFWwindow *window, int width,
                                          int height);
};
