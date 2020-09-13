#pragma once

#include "Omnia/Omnia.pch"
#include "Omnia/Core.h"

#include "Vulkan.h"

namespace Omnia {

struct VKQueueFamilyIndices {
    int32_t Graphics = -1;
    int32_t Compute = -1;
    int32_t Transfer = -1;
};

class VKPhysicalDevice {
    friend class VKContext; // ToDo: Only needed while porting code
    friend class VKDevice;

public:
    VKPhysicalDevice();
    ~VKPhysicalDevice();

    static Reference<VKPhysicalDevice> Select();

    vk::PhysicalDevice GePhysicalDevice() const;
    uint32_t GetMemoryTypeIndex(uint32_t bits, vk::MemoryPropertyFlags properties);

    bool IsExtensionSupport(const string &name) const;

private:
    VKQueueFamilyIndices GetQueueFamilyIndices(int32_t flags);

private:
    vector<vk::PhysicalDevice> mPhysicalDevices = {};
    std::unordered_set<string> mSupportedExtensions = {};

    vk::PhysicalDevice mPhysicalDevice = nullptr;
    vk::PhysicalDeviceFeatures mFeatures = {};
    vk::PhysicalDeviceProperties mProperties = {};
    vk::PhysicalDeviceMemoryProperties mMemoryProperties = {};

    VKQueueFamilyIndices mQueueFamilyIndices;
    vector<vk::QueueFamilyProperties> mQueueFamilyProperties;
    vector<vk::DeviceQueueCreateInfo> mQueueCreateInformation;
};

class VKDevice {
    friend class VKContext; // ToDo: Only needed while porting code
public:
    VKDevice(const Reference<VKPhysicalDevice> &physicalDeice);
    ~VKDevice();

    vk::Device GetDevice() const;
    vk::CommandBuffer GetCommandBuffer(bool start);
    vk::Queue GetQueue();
    const Reference<VKPhysicalDevice> &GetPhysicalDevice() const;

    void FlushCommandBuffer(vk::CommandBuffer buffer);

private:
    vk::Device mDevice = nullptr;
    vk::CommandPool mCommandPool;
    vk::Queue mQueue;

    Reference<VKPhysicalDevice> mPhysicalDevice;
};

}
