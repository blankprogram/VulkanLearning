#include "engine/core/DebugUtilsMessenger.hpp"
#include <iostream>

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void * /*pUserData*/) {
    std::cerr << "[Vulkan] " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

vk::DebugUtilsMessengerCreateInfoEXT createDebugUtilsMessengerCreateInfo() {
    return vk::DebugUtilsMessengerCreateInfoEXT{}
        .setMessageSeverity(
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
        .setMessageType(
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
        .setPfnUserCallback(debugCallback)
        .setPUserData(nullptr);
}

} // namespace

namespace engine {

DebugUtilsMessenger::DebugUtilsMessenger(const vk::raii::Instance &instance)
    : messenger_(instance, createDebugUtilsMessengerCreateInfo()) {}

} // namespace engine
