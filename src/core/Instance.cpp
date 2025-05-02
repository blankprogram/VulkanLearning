#include "engine/core/Instance.hpp"
#include "engine/configs/ValidationConfig.hpp"

namespace engine {

static vk::raii::Instance createInstance(vk::raii::Context &context) {
    vk::ApplicationInfo appInfo{};
    appInfo.pApplicationName = validationConfig.applicationName.c_str();
    appInfo.applicationVersion = validationConfig.applicationVersion;
    appInfo.pEngineName = validationConfig.engineName.c_str();
    appInfo.engineVersion = validationConfig.engineVersion;
    appInfo.apiVersion = validationConfig.apiVersion;

    vk::InstanceCreateInfo createInfo{};
    createInfo.pApplicationInfo = &appInfo;

    if (validationConfig.enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(
            validationConfig.validationLayers.size());
        createInfo.ppEnabledLayerNames =
            validationConfig.validationLayers.data();
    }

    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(validationConfig.enabledExtensions.size());
    createInfo.ppEnabledExtensionNames =
        validationConfig.enabledExtensions.data();

    return vk::raii::Instance{context, createInfo};
}

Instance::Instance(vk::raii::Context &context)
    : validationEnabled_(validationConfig.enableValidationLayers),
      instance_(createInstance(context)) {
    if (validationEnabled_) {
        debugMessenger_ = std::make_unique<DebugUtilsMessenger>(instance_);
    }
}

const vk::raii::Instance &Instance::get() const { return instance_; }

bool Instance::isValidationEnabled() const { return validationEnabled_; }

} // namespace engine
