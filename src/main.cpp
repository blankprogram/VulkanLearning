
#include "engine/app/Window.hpp"
#include "engine/configs/WindowConfig.hpp"
#include "engine/core/Context.hpp"
#include "engine/core/Instance.hpp"
#include <iostream>
#include <vulkan/vulkan_raii.hpp>

int main() {
    try {
        vk::raii::Context context;

        engine::Instance instance(context);

        engine::Window window(engine::windowConfig.width, engine::windowConfig.height, engine::windowConfig.name);

        while (!window.shouldClose()) {
            glfwPollEvents();
        }

        return 0;

    } catch (const std::exception &e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }
}
