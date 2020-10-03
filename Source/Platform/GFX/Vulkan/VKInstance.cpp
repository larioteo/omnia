#include "VKInstance.h"

#include "Omnia/Log.h"

PFN_vkCreateDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT DestroyDebugUtilsMessengerEXT;
VkDebugUtilsMessengerEXT dbg_messenger;

// Define a callback to capture the messages
VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessagerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {

    char prefix[64];
    char *message = (char *)malloc(strlen(callbackData->pMessage) + 500);
    assert(message);

    //    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
    //        strcpy(prefix, "VERBOSE : ");
    //    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    //        strcpy(prefix, "INFO : ");
    //    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    //        strcpy(prefix, "WARNING : ");
    //    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    //        strcpy(prefix, "ERROR : ");
    //    }
    switch ((vk::DebugUtilsMessageSeverityFlagBitsEXT)messageSeverity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError: {
            Omnia::AppLogError("[GFX::Vulkan] ", callbackData->pMessageIdName, ": ", callbackData->pMessage);
            break;
        }
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo: {
            Omnia::AppLogInfo("[GFX::Vulkan] ", callbackData->pMessageIdName, ": ", callbackData->pMessage);
            break;
        }
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose: {
            Omnia::AppLogTrace("[GFX::Vulkan] ", callbackData->pMessageIdName, ": ", callbackData->pMessage);
            break;
        }
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: {
            Omnia::AppLogWarning("[GFX::Vulkan] ", callbackData->pMessageIdName, ": ", callbackData->pMessage);
            break;
        }
    }
    
//    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
//        strcat(prefix, "GENERAL");
//    } else {
//        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_SPECIFICATION_BIT_EXT) {
//            strcat(prefix, "SPEC");
//            validation_error = 1;
//        }
//        if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
//            if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_SPECIFICATION_BIT_EXT) {
//                strcat(prefix, "|");
//            }
//            strcat(prefix, "PERF");
//        }
//
//    }
//sprintf(message,
//        "%s - Message ID Number %d, Message ID String :\n%s",
//        prefix,
//        callbackData->messageIdNumber,
//        callbackData->pMessageIdName,
//        callbackData->pMessage);
//if (callbackData->objectCount > 0) {
//    char tmp_message[500];
//    sprintf(tmp_message, "\n Objects - %d\n", callbackData->objectCount);
//    strcat(message, tmp_message);
//    for (uint32_t object = 0; object < callbackData->objectCount; ++object) {
//        sprintf(tmp_message,
//                " Object[%d] - Type %s, Value %p, Name \"%s\"\n",
//                Object,
//                DebugAnnotObjectToString(
//                    callbackData->pObjects[object].objectType),
//                (void*)(callbackData->pObjects[object].objectHandle),
//                callbackData->pObjects[object].pObjectName);
//        strcat(message, tmp_message);
//    }
//}
//if (callbackData->cmdBufLabelCount > 0) {
//    char tmp_message[500];
//    sprintf(tmp_message,
//            "\n Command Buffer Labels - %d\n",
//            callbackData->cmdBufLabelCount);
//    strcat(message, tmp_message);
//    for (uint32_t label = 0; label < callbackData->cmdBufLabelCount; ++label) {
//        sprintf(tmp_message,
//                " Label[%d] - %s { %f, %f, %f, %f}\n",
//                Label,
//                callbackData->pCmdBufLabels[label].pLabelName,
//                callbackData->pCmdBufLabels[label].color[0],
//                callbackData->pCmdBufLabels[label].color[1],
//                callbackData->pCmdBufLabels[label].color[2],
//                callbackData->pCmdBufLabels[label].color[3]);
//        strcat(message, tmp_message);
//    }
//}
//
//printf("%s\n", message);
//fflush(stdout);
//free(message);
//// Don't bail out, but keep going.
    return false;
}

namespace Omnia {

// Default
VKInstance::VKInstance() {
    // Application Information
    vk::ApplicationInfo applicationInfo = {
        "Application",  VK_MAKE_VERSION(1, 0, 0),
        "Engine",       VK_MAKE_VERSION(1, 0, 0),
        /*API*/         VK_API_VERSION_1_2,
    };

    // Extensions
    vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
    vector<const char *> neededExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        #ifdef APP_PLATFORM_WINDOWS
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
        #endif
        #ifdef APP_DEBUG_MODE
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        #endif
    };
    vector<const char *> extensions = GetExtensions(availableExtensions, neededExtensions);

    // Layers
    vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
    vector<const char *> neededLayers = {
        "VK_LAYER_LUNARG_standard_validation",
        "VK_LAYER_KHRONOS_validation"
    };
    vector<const char *> layers = GetLayers(availableLayers, neededLayers);

    // Instance
    vk::InstanceCreateInfo intanceCreateInfo = {
        vk::InstanceCreateFlags(),
        &applicationInfo,
        layers,
        extensions
    };
    try {
        mInstance = vk::createInstance(intanceCreateInfo);
    } catch (const std::exception& e) {
        AppLogCritical("[GFX::Instance] ", e.what());
    }

    // Debug
    CreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT");
    DestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");

    //vk::DebugUtilsMessengerCreateInfoEXT debugUtilMessangerCreateInfo = {};
    //debugUtilMessangerCreateInfo.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning;
    //debugUtilMessangerCreateInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;
    //debugUtilMessangerCreateInfo.pfnUserCallback = &VulkanDebugMessagerCallback;
    //auto mDebugUtilsMessenger = mInstance.createDebugUtilsMessengerEXTUnique(debugUtilMessangerCreateInfo);
    // Setup our pointers to the VK_EXT_debug_utils commands
    
    VkDebugUtilsMessengerCreateInfoEXT debugMessangerCreateInfo;
    debugMessangerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugMessangerCreateInfo.pNext = NULL;
    debugMessangerCreateInfo.flags = 0;
    debugMessangerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    debugMessangerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugMessangerCreateInfo.pfnUserCallback = DebugMessagerCallback;
    debugMessangerCreateInfo.pUserData = NULL;
    VkResult result = CreateDebugUtilsMessengerEXT(mInstance, &debugMessangerCreateInfo, NULL, &mDebugUtilsMessanger);
}

VKInstance::~VKInstance() {
    if (mInstance) {
        mInstance.destroy();
    }
    if (mDebugUtilsMessanger) {
        DestroyDebugUtilsMessengerEXT(mInstance, mDebugUtilsMessanger, NULL);
    }
}

// Accessors
const vk::Instance &VKInstance::Call() const {
    return mInstance;
}

// Conversions
VKInstance::operator const vk::Instance &() const  {
    return mInstance;
}

// Internal
vector<const char *> VKInstance::GetExtensions(const vector<vk::ExtensionProperties> &available, const vector<const char *> &needed) {
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

vector<const char *> VKInstance::GetLayers(const vector<vk::LayerProperties> &available, const vector<const char *> &needed) {
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

}
