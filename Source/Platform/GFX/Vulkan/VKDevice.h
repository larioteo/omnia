#pragma once

#include "Omnia/Omnia.pch"
#include "Omnia/Core.h"

#include "Vulkan.h"

namespace Ultra {
class VKTest;
}

namespace Omnia {

struct VKQueueFamilyIndices {
    int32_t Compute = -1;
    int32_t Graphics = -1;
    int32_t Protected = -1;
    int32_t SparseBinding = -1;
    int32_t Transfer = -1;
};

class VKPhysicalDevice {
    friend Ultra::VKTest;
    friend class VKContext; // ToDo: Only needed while porting code
    friend class VKDevice;
    friend class VKSwapChain; // ToDo: Only needed while porting code

public:
    explicit VKPhysicalDevice(const vk::Instance &instance);
    ~VKPhysicalDevice() = default;

    // Accessors
    const vk::PhysicalDevice &GePhysicalDevice() const;
    const vk::PhysicalDeviceFeatures &GetFeatures() const;
    const vk::PhysicalDeviceProperties &GetProperties() const;
    const vk::PhysicalDeviceMemoryProperties &GetMemoryProperties() const;

    // Conversions
    operator const vk::PhysicalDevice &() const;
    operator const string() const;

    // Queries
    bool IsExtensionSupport(const string &name) const;

private:
    // Internal
    vk::PhysicalDevice ChoosePhysicalDevice(const vector<vk::PhysicalDevice> &devices);
    uint32_t GetPhysicalDeviceRanking(const vk::PhysicalDevice &device);
    uint32_t GetMemoryTypeIndex(uint32_t bits, vk::MemoryPropertyFlags properties);
    VKQueueFamilyIndices GetQueueFamilyIndices(vk::QueueFlags flags);

private:
    const vk::Instance mInstance;
    static inline const float DefaultPriority = 0.0f;

    vector<vk::PhysicalDevice> mPhysicalDevices = {};
    std::unordered_set<string> mSupportedExtensions = {};

    vk::PhysicalDevice mPhysicalDevice = nullptr;
    vk::PhysicalDeviceFeatures mFeatures = {};
    vk::PhysicalDeviceProperties mProperties = {};
    vk::PhysicalDeviceMemoryProperties mMemoryProperties = {};

    VKQueueFamilyIndices mQueueFamilyIndices;
    vector<vk::DeviceQueueCreateInfo> mQueueCreateInformation;
    vector<vk::QueueFamilyProperties> mQueueFamilyProperties;
};

class VKDevice {
    friend Ultra::VKTest;
    friend class VKContext; // ToDo: Only needed while porting code
    friend class VKSwapChain; // ToDo: Only needed while porting code

public:
    VKDevice(const Reference<VKPhysicalDevice> &physicalDevice);
    ~VKDevice() = default;
    void Destroy();

    // Accessors
    const vk::Device &GetDevice() const;
    const vk::CommandBuffer &GetCommandBuffer(bool start = false) const;
    const vk::Queue &GetQueue() const;
    const Reference<VKPhysicalDevice> &GetPhysicalDevice() const;

    // Conversions
    operator const vk::Device &() const;

    // Commands
    void FlushCommandBuffer(vk::CommandBuffer &buffer);

private:
    Reference<VKPhysicalDevice> mPhysicalDevice;

    vk::Device mDevice = nullptr;
    vk::CommandPool mCommandPool;
    vk::Queue mQueue;
};

}
