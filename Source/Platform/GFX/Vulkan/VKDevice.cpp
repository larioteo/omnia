#include "VKDevice.h"

#include "Omnia/Log.h"

// ToDo: Feature to support multiple physical devices

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

/*
 * Physical Device
*/
VKPhysicalDevice::VKPhysicalDevice(const vk::Instance &instance): mInstance(instance) {
    // Get available physical devices and choose the "best" one
    mPhysicalDevices = instance.enumeratePhysicalDevices();
    mPhysicalDevice = ChoosePhysicalDevice(mPhysicalDevices);
    AppAssert(mPhysicalDevice, "GFX:VKPhysicalDevice: ", "No suiteable vulkan supporting deivce found!");
    
    // Get features and properties
    mFeatures = mPhysicalDevice.getFeatures();
    mMemoryProperties = mPhysicalDevice.getMemoryProperties();
    mProperties = mPhysicalDevice.getProperties();
    mQueueFamilyProperties = mPhysicalDevice.getQueueFamilyProperties();
    for (auto &extension : mPhysicalDevice.enumerateDeviceExtensionProperties()) {
        mSupportedExtensions.emplace(extension.extensionName.data());
        //AppLogTrace(extension.extensionName);
    }

    // Get Queues
    vk::QueueFlags requestedQueueTypes = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eTransfer;
    mQueueFamilyIndices = GetQueueFamilyIndices(requestedQueueTypes);
    if (requestedQueueTypes & vk::QueueFlagBits::eCompute) {
        vk::DeviceQueueCreateInfo info {};
        info.queueFamilyIndex = mQueueFamilyIndices.Compute;
        info.queueCount = 1;
        info.pQueuePriorities = &DefaultPriority;
        mQueueCreateInformation.push_back(info);
    }
    if (requestedQueueTypes & vk::QueueFlagBits::eGraphics) {
        vk::DeviceQueueCreateInfo info {};
        info.queueFamilyIndex = mQueueFamilyIndices.Graphics;
        info.queueCount = 1;
        info.pQueuePriorities = &DefaultPriority;
        mQueueCreateInformation.push_back(info);
    }
    if (requestedQueueTypes & vk::QueueFlagBits::eProtected) {
        vk::DeviceQueueCreateInfo info {};
        info.queueFamilyIndex = mQueueFamilyIndices.Protected;
        info.queueCount = 1;
        info.pQueuePriorities = &DefaultPriority;
        mQueueCreateInformation.push_back(info);
    }
    if (requestedQueueTypes & vk::QueueFlagBits::eSparseBinding) {
        vk::DeviceQueueCreateInfo info {};
        info.queueFamilyIndex = mQueueFamilyIndices.SparseBinding;
        info.queueCount = 1;
        info.pQueuePriorities = &DefaultPriority;
        mQueueCreateInformation.push_back(info);
    }
    if (requestedQueueTypes & vk::QueueFlagBits::eTransfer) {
        vk::DeviceQueueCreateInfo info {};
        info.queueFamilyIndex = mQueueFamilyIndices.Transfer;
        info.queueCount = 1;
        info.pQueuePriorities = &DefaultPriority;
        mQueueCreateInformation.push_back(info);
    }
    AppLogTrace("GFX:VKPhysicalDevice: ", (string)*this);
}

// Accessors
const vk::PhysicalDevice &VKPhysicalDevice::GePhysicalDevice() const {
    return mPhysicalDevice;
}

const vk::PhysicalDeviceFeatures &VKPhysicalDevice::GetFeatures() const {
    return mFeatures;
}

const vk::PhysicalDeviceProperties &VKPhysicalDevice::GetProperties() const {
    return mProperties;
}

const vk::PhysicalDeviceMemoryProperties &VKPhysicalDevice::GetMemoryProperties() const {
    return mMemoryProperties;
}

// Conversions
VKPhysicalDevice::operator const vk::PhysicalDevice &() const {
    return mPhysicalDevice;
}

VKPhysicalDevice::operator const string() const {
    stringstream stream;

    // Caption
    stream << "Selected physical device '" << mProperties.deviceName << "' [";

    // Vendor ID
    stream << "VendorID='";
    switch (mProperties.vendorID) {
        case 0x1002:    { stream << "AMD";      break; }
        case 0x8086:    { stream << "Intel";    break; }
        case 0x10DE:    { stream << "nVidia";   break; }
        default:        { stream << "#" << mProperties.vendorID; break; }
    }
    stream << "' | ";

    // Device Type
    stream << "Type='";
    switch (mProperties.deviceType) {
        case vk::PhysicalDeviceType::eCpu:              { stream << "CPU";    break; }
        case vk::PhysicalDeviceType::eDiscreteGpu:      { stream << "GPU";    break; }
        case vk::PhysicalDeviceType::eIntegratedGpu:    { stream << "I-GPU";  break; }
        case vk::PhysicalDeviceType::eVirtualGpu:       { stream << "V-GPU";  break; }
        default:                                        { stream << "Other";  break; }
    }
    stream << "'";
    
    stream << "]";
    return stream.str();
}

// Queries
bool VKPhysicalDevice::IsExtensionSupport(const string &name) const {
    //return mSupportedExtensions.find(name) != mSupportedExtensions.end();
    return false;
}

// Internal
vk::PhysicalDevice VKPhysicalDevice::ChoosePhysicalDevice(const vector<vk::PhysicalDevice> &devices) {
    std::multimap<uint32_t, vk::PhysicalDevice> rankingList;
    for (const auto &device : devices) {
        rankingList.emplace(GetPhysicalDeviceRanking(device), device);
    }

    if (rankingList.rbegin()->first > 0) return rankingList.rbegin()->second;
    return nullptr;
}

uint32_t VKPhysicalDevice::GetPhysicalDeviceRanking(const vk::PhysicalDevice &device) {
    // ToDo:: Choose device which supports all extensions that are needed, rather than basically counting how many are supported
    uint32_t score = 0;
    vector<vk::ExtensionProperties> extensionProperties = device.enumerateDeviceExtensionProperties();

    auto &properties = device.getProperties();
    if (mProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) score += 1024;
    score += extensionProperties.size();

    return score;
}

VKQueueFamilyIndices VKPhysicalDevice::GetQueueFamilyIndices(vk::QueueFlags flags) {
    VKQueueFamilyIndices indices = {};

    if (flags & vk::QueueFlagBits::eTransfer) {
        for (uint32_t i = 0; i < mQueueFamilyProperties.size(); i++) {
            auto &properties = mQueueFamilyProperties[i];
            if ((properties.queueFlags & vk::QueueFlagBits::eTransfer) && !(properties.queueFlags & vk::QueueFlagBits::eGraphics) && !(properties.queueFlags & vk::QueueFlagBits::eCompute)) {
                indices.Transfer = i;
                break;
            }
        }
    }

    if (flags & vk::QueueFlagBits::eCompute) {
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
        if (flags & vk::QueueFlagBits::eTransfer && indices.Transfer == -1) {
            if (properties.queueFlags & vk::QueueFlagBits::eTransfer) indices.Transfer = i;
        }
        if (flags & vk::QueueFlagBits::eCompute && indices.Compute == -1) {
            if (properties.queueFlags & vk::QueueFlagBits::eCompute) indices.Compute = i;
        }
        if (flags & vk::QueueFlagBits::eGraphics) {
            if (properties.queueFlags & vk::QueueFlagBits::eGraphics) indices.Graphics = i;
        }
    }

    return indices;
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



/*
* Logical Device
*/

VKDevice::VKDevice(const Reference<VKPhysicalDevice> &physicalDevice):
    mPhysicalDevice { physicalDevice } {

    vector<vk::ExtensionProperties> availableDeviceExtensions = mPhysicalDevice->GePhysicalDevice().enumerateDeviceExtensionProperties();
    vector<const char*> neededDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    std::vector<const char*> deviceExtensions = GetExtensions(availableDeviceExtensions, neededDeviceExtensions);

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.setQueueCreateInfoCount(static_cast<uint32_t>(mPhysicalDevice->mQueueCreateInformation.size()));
    deviceCreateInfo.setPQueueCreateInfos(mPhysicalDevice->mQueueCreateInformation.data());
    deviceCreateInfo.setEnabledExtensionCount(static_cast<uint32_t>(deviceExtensions.size()));
    deviceCreateInfo.setPpEnabledExtensionNames(deviceExtensions.data());
    mDevice = mPhysicalDevice->GePhysicalDevice().createDevice(deviceCreateInfo);

    vk::PhysicalDeviceFeatures2 features2 = {};
    mDevice = mPhysicalDevice->GePhysicalDevice().createDevice(deviceCreateInfo);

    vk::CommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.queueFamilyIndex = mPhysicalDevice->mQueueFamilyIndices.Graphics;
    commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    mCommandPool = mDevice.createCommandPool(commandPoolCreateInfo);

    mQueue = mDevice.getQueue(mPhysicalDevice->mQueueFamilyIndices.Graphics, 0);
}

void VKDevice::Destroy() {
    mDevice.destroyCommandPool(mCommandPool);
    mDevice.destroy();
}

// Accessors
const vk::Device &VKDevice::GetDevice() const {
    return mDevice;
}

const vk::CommandBuffer &VKDevice::GetCommandBuffer(bool start) const  {
    vk::CommandBuffer buffer = nullptr;
    vk::CommandBufferAllocateInfo bufferAllcoateInfo = {};
    bufferAllcoateInfo.commandPool = mCommandPool;
    bufferAllcoateInfo.level = vk::CommandBufferLevel::ePrimary;
    bufferAllcoateInfo.commandBufferCount = 1;

    buffer = (mDevice.allocateCommandBuffers(bufferAllcoateInfo))[0];

    if (start) {
        vk::CommandBufferBeginInfo bufferBeginInfo = {};
        buffer.begin(bufferBeginInfo);
    }

    return buffer;
}

const vk::Queue &VKDevice::GetQueue() const {
    return mQueue;
}

const Reference<VKPhysicalDevice> &VKDevice::GetPhysicalDevice() const {
    return mPhysicalDevice;
}

// Conversions
VKDevice::operator const vk::Device &() const {
    return mDevice;
}

// Commands
void VKDevice::FlushCommandBuffer(vk::CommandBuffer &buffer) {
    const uint64_t DefaultFenceTimeout = UINT64_MAX;
    buffer.end();

    vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buffer;
    submitInfo.signalSemaphoreCount = 0;
    submitInfo.pSignalSemaphores = nullptr;
    submitInfo.pWaitSemaphores = nullptr;
    submitInfo.pWaitDstStageMask = &waitDstStageMask;


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
