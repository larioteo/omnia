#pragma once

#include "Omnia/Omnia.pch"
#include "Omnia/Core.h"

#include "Vulkan.h"

namespace Omnia {

class VKInstance {
public:
    VKInstance() = default;
    ~VKInstance() = default;

    void Load();
    vk::Instance Call();

    operator vk::Instance();

private:
    Reference<vk::Instance> mInstance = nullptr;
    static inline vk::InstanceCreateInfo mProperties = {};
};

}
