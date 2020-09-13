#pragma once

#include "Omnia/Omnia.pch"
#include "Omnia/Core.h"

#include "Vulkan.h"

namespace Omnia {

class VKInstance {
public:
    VKInstance();
    ~VKInstance() = default;

    vk::Instance Call();

    operator vk::Instance();

private:
    vk::Instance mInstance = nullptr;
    static inline vk::InstanceCreateInfo mProperties = {};
};

}
