
#include "engine/platform/WindowManager.hpp"
#include <stdexcept>
bool WindowManager::framebufferResized = false;
WindowManager::WindowManager(uint32_t width, uint32_t height,
                             const std::string &title) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    window_ = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    setupCallbacks();
}

WindowManager::~WindowManager() {
    glfwDestroyWindow(window_);
    glfwTerminate();
}

GLFWwindow *WindowManager::getWindow() { return window_; }

bool WindowManager::shouldClose() { return glfwWindowShouldClose(window_); }

void WindowManager::pollEvents() { glfwPollEvents(); }

void WindowManager::setupCallbacks() {
    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
}

void WindowManager::framebufferResizeCallback(GLFWwindow *window, int width,
                                              int height) {
    framebufferResized = true;
}
