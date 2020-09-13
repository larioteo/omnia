#include "VKInstance.h"

namespace Omnia {

vector<const char *> GetExtensions(const vector<vk::ExtensionProperties> &available, const vector<const char*> &needed) {
    vector<const char *> result = {};
    for (auto const &n : needed) {
        for (auto const &a : available) {
            if (string(a.extensionName.data()).compare(n) == 0) {
                result.emplace_back(n);
                break;
            }
        }
    }
    return result;
}

vector<const char *> GetLayers(const std::vector<vk::LayerProperties> &available, const std::vector<const char*> &needed) {
    vector<const char *> result = {};
    for (auto const &n : needed) {
        for (auto const &a : available) {
            if (string(a.layerName.data()).compare(n) == 0) {
                result.emplace_back(n);
                break;
            }
        }
    }
    return result;
}


void VKInstance::Load() {
    // Application Information
    vk::ApplicationInfo applicationInfo;
    applicationInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = NULL,
        .pApplicationName   = "App",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 0),
        .pEngineName        = "Engine",
        .engineVersion      = VK_MAKE_VERSION(0, 0, 0),
        .apiVersion         = VK_API_VERSION_1_2,
    };
    
    // Extensions
    vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
    vector<const char *> neededExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        #ifdef APP_DEBUG_MODE
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        #endif
        #ifdef VK_USE_PLATFORM_WIN32_KHR
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME
        #endif
    };
    vector<const char *> extensions = GetExtensions(availableExtensions, neededExtensions);

    // Layers
    vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
    vector<const char *> neededLayers = {
        "VK_LAYER_LUNARG_standard_validation"
    };
    vector<const char *> layers = GetLayers(availableLayers, neededLayers);

    // Properties
    mProperties = {
        .sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                      = NULL,
        .flags                      = 0,
        //.pApplicationInfo           = &applicationInfo,
        .enabledLayerCount          = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames        = layers.data(),
        .enabledExtensionCount      = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames    = extensions.data(),
    };
    mProperties.pApplicationInfo = &applicationInfo;

    // Create Instance
    mInstance = CreateReference<vk::Instance>(vk::createInstance(mProperties));
}

vk::Instance VKInstance::Call() {
    return *mInstance;
}


VKInstance::operator vk::Instance() {
    return *mInstance;
}

}
