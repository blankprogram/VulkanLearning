#pragma once
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace engine {

struct ValidationConfig {
    bool enableValidationLayers = true;
    std::vector<const char *> validationLayers = {
        "VK_LAYER_KHRONOS_validation"};
    std::vector<const char *> enabledExtensions = {
        "VK_KHR_surface", "VK_KHR_xcb_surface", "VK_EXT_debug_utils"};
    std::string applicationName = "VulkanLearning";
    std::string engineName = "VulkanEngine";
    uint32_t applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    uint32_t engineVersion = VK_MAKE_VERSION(1, 0, 0);
    uint32_t apiVersion = VK_API_VERSION_1_3;
};

inline const ValidationConfig validationConfig{};

} // namespace engine
