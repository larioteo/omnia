#include "VKDevice.h"

#include "VKContext.h"
#include "Omnia/Log.h"

namespace Omnia {

namespace {

// ToDo: Not needed anymore but maybe usefull
uint32_t GetQueueIndex(vk::PhysicalDevice& physicalDevice, vk::QueueFlagBits flags) {
    vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();
    for (size_t i = 0; i < queueProps.size(); ++i) {
        if (queueProps[i].queueFlags & flags) {
            return static_cast<uint32_t>(i);
        }
    }
    return 0;
}

vector<const char *> GetExtensions(const vector<vk::ExtensionProperties>& available, const vector<const char*>& needed) {
    vector<const char *> result = {};
    for (const char* const& n : needed) {
        for (vk::ExtensionProperties const &a : available) {
            if (string(a.extensionName.data()).compare(n) == 0) {
                result.emplace_back(n);
                break;
            }
        }
    }
    return result;
}

}


VKPhysicalDevice::VKPhysicalDevice() {
    auto &instance = VKContext::GetInstance();

    // Get a vulkan supporting GPU
    mPhysicalDevices = instance.enumeratePhysicalDevices();
    for (auto &device : mPhysicalDevices) {
        mProperties = device.getProperties();
        // ToDo: Prefer discrete GPU but support also everything else
        if (mProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            mPhysicalDevice = device;
            if (mPhysicalDevice) break;
        }
    }

    mFeatures = mPhysicalDevice.getFeatures();
    mMemoryProperties = mPhysicalDevice.getMemoryProperties();
    mQueueFamilyProperties = mPhysicalDevice.getQueueFamilyProperties();

    for (auto &extension : mPhysicalDevice.enumerateDeviceExtensionProperties()) {
        mSupportedExtensions.emplace(extension.extensionName.data());
        //AppLogTrace(extension.extensionName);
    }

    static const float defaultPriority = 0.0f;
    int requestedQueueTypes = (int)vk::QueueFlagBits::eGraphics | (int)vk::QueueFlagBits::eCompute | (int)vk::QueueFlagBits::eTransfer;
    mQueueFamilyIndices = GetQueueFamilyIndices(requestedQueueTypes);

    if (requestedQueueTypes & (int)vk::QueueFlagBits::eGraphics) {
        vk::DeviceQueueCreateInfo info {};
        info.queueFamilyIndex = mQueueFamilyIndices.Graphics;
        info.queueCount = 1;
        info.pQueuePriorities = &defaultPriority;
        mQueueCreateInformation.push_back(info);
    }

    if (requestedQueueTypes & (int)vk::QueueFlagBits::eCompute) {
        vk::DeviceQueueCreateInfo info {};
        info.queueFamilyIndex = mQueueFamilyIndices.Compute;
        info.queueCount = 1;
        info.pQueuePriorities = &defaultPriority;
        mQueueCreateInformation.push_back(info);
    }

    if (requestedQueueTypes & (int)vk::QueueFlagBits::eTransfer) {
        vk::DeviceQueueCreateInfo info {};
        info.queueFamilyIndex = mQueueFamilyIndices.Transfer;
        info.queueCount = 1;
        info.pQueuePriorities = &defaultPriority;
        mQueueCreateInformation.push_back(info);
    }
}

VKPhysicalDevice::~VKPhysicalDevice() {}

Reference<VKPhysicalDevice> VKPhysicalDevice::Select() {
    return Reference<VKPhysicalDevice>();
}

VKQueueFamilyIndices VKPhysicalDevice::GetQueueFamilyIndices(int32_t flags) {
    VKQueueFamilyIndices indices = {};

    if (flags & (int)vk::QueueFlagBits::eTransfer) {
        for (uint32_t i = 0; i < mQueueFamilyProperties.size(); i++) {
            auto &properties = mQueueFamilyProperties[i];
            if ((properties.queueFlags & vk::QueueFlagBits::eTransfer) && !(properties.queueFlags & vk::QueueFlagBits::eGraphics) && !(properties.queueFlags & vk::QueueFlagBits::eCompute)) {
                indices.Transfer = i;
                break;
            }
        }
    }

    if (flags & (int)vk::QueueFlagBits::eCompute) {
        for (uint32_t i = 0; i < mQueueFamilyProperties.size(); i++) {
            auto &properties = mQueueFamilyProperties[i];
            if ((properties.queueFlags & vk::QueueFlagBits::eCompute) && !(properties.queueFlags & vk::QueueFlagBits::eGraphics)) {
                indices.Compute = i;
                break;
            }
        }
    }

    for (uint32_t i = 0; i < mQueueFamilyProperties.size(); i++) {
        auto &properties = mQueueFamilyProperties[i];
        if (flags & (int)vk::QueueFlagBits::eTransfer && indices.Transfer == -1) {
            if (properties.queueFlags & vk::QueueFlagBits::eTransfer) indices.Transfer = i;
        }
        if (flags & (int)vk::QueueFlagBits::eCompute && indices.Compute == -1) {
            if (properties.queueFlags & vk::QueueFlagBits::eCompute) indices.Compute = i;
        }
        if (flags & (int)vk::QueueFlagBits::eGraphics) {
            if (properties.queueFlags & vk::QueueFlagBits::eGraphics) indices.Graphics = i;
        }
    }

    return indices;
}

vk::PhysicalDevice VKPhysicalDevice::GePhysicalDevice() const {
    return mPhysicalDevice;
}

uint32_t VKPhysicalDevice::GetMemoryTypeIndex(uint32_t bits, vk::MemoryPropertyFlags properties) {
    for (uint32_t i = 0; i < mMemoryProperties.memoryTypeCount; i++) {
        if ((bits & 1) == 1) {
            if ((mMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        bits >>= 1;
    }
    return UINT32_MAX;
}

bool VKPhysicalDevice::IsExtensionSupport(const string &name) const {
    //return mSupportedExtensions.find(name) != mSupportedExtensions.end();
    return false;
}



VKDevice::VKDevice(const Reference<VKPhysicalDevice> &physicalDeice):
    mPhysicalDevice { physicalDeice } {

    vector<vk::ExtensionProperties> availableDeviceExtensions = mPhysicalDevice->GePhysicalDevice().enumerateDeviceExtensionProperties();
    vector<const char*> neededDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    std::vector<const char*> deviceExtensions = GetExtensions(availableDeviceExtensions, neededDeviceExtensions);

    vk::DeviceCreateInfo dinfo;
    dinfo.setQueueCreateInfoCount(static_cast<uint32_t>(mPhysicalDevice->mQueueCreateInformation.size()));
    dinfo.setPQueueCreateInfos(mPhysicalDevice->mQueueCreateInformation.data());
    dinfo.setEnabledExtensionCount(static_cast<uint32_t>(deviceExtensions.size()));
    dinfo.setPpEnabledExtensionNames(deviceExtensions.data());
    mDevice = mPhysicalDevice->GePhysicalDevice().createDevice(dinfo);

    vk::PhysicalDeviceFeatures2 features2 = {};
    mDevice = mPhysicalDevice->GePhysicalDevice().createDevice(dinfo);

    vk::CommandPoolCreateInfo poolInfo = {};
    poolInfo.queueFamilyIndex = mPhysicalDevice->mQueueFamilyIndices.Graphics;
    poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    mCommandPool = mDevice.createCommandPool(poolInfo);
    mQueue = mDevice.getQueue(mPhysicalDevice->mQueueFamilyIndices.Graphics, 0);
}

VKDevice::~VKDevice() {}

vk::Device VKDevice::GetDevice() const {
    return mDevice;
}

vk::CommandBuffer VKDevice::GetCommandBuffer(bool start) {
    vk::CommandBuffer buffer = nullptr;
    vk::CommandBufferAllocateInfo bufferAlcoateInfo = {};
    bufferAlcoateInfo.commandPool = mCommandPool;
    bufferAlcoateInfo.level = vk::CommandBufferLevel::ePrimary;
    bufferAlcoateInfo.commandBufferCount = 1;

    buffer = (mDevice.allocateCommandBuffers(bufferAlcoateInfo))[0];

    if (start) {
        vk::CommandBufferBeginInfo bufferBeginInfo = {};
        buffer.begin(bufferBeginInfo);
    }

    return buffer;
}

vk::Queue VKDevice::GetQueue() {
    return mQueue;
}

const Reference<VKPhysicalDevice> &VKDevice::GetPhysicalDevice() const {
    return mPhysicalDevice;
}

void VKDevice::FlushCommandBuffer(vk::CommandBuffer buffer) {
    const uint64_t DefaultFenceTimeout = 1000000000000;
    buffer.end();

    vk::SubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buffer;

    vk::FenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    vk::Fence fence = nullptr;
    fence = mDevice.createFence(fenceCreateInfo, nullptr);
    mQueue.submit(submitInfo, fence);
    mDevice.waitForFences(fence, VK_TRUE, DefaultFenceTimeout);
    mDevice.destroyFence(fence, nullptr);
    mDevice.freeCommandBuffers(mCommandPool, buffer);
}

}
