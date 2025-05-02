#include "engine/core/Context.hpp"
#include "engine/core/Instance.hpp"
#include <iostream>
#include <vulkan/vulkan_raii.hpp>

int main() {
    try {
        vk::raii::Context context;

        engine::Instance instance(context);

        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }
}
