#pragma once

#include "Omnia/GFX/Context.h"
#include "Omnia/Utility/Timer.h"

#if defined(APP_PLATFORM_WINDOWS)
    #define VC_EXTRALEAN
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #undef APIENTRY
    #include <Windows.h>

    #define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.hpp>

#include "VKInstance.h"
#include "VKDevice.h"
#include "VKSwapChain.h"

namespace Ultra {
class VKTest;
}

namespace Omnia {

struct VkContextData {
    vk::Instance Intance;
    vk::PhysicalDevice PhysicalDevice;
    vk::Device Device;
    uint32_t QueueIndex;
    vk::Queue Queue;
    vk::PipelineCache PipelineCache;
    vk::DescriptorPool DescriptorPool;
    vk::AllocationCallbacks Allocator = nullptr;
    uint32_t MinImageCount = 2;
    uint32_t ImageCount = 16;
    vk::RenderPass RenderPass;
    vk::SurfaceKHR Surface;
    vk::SwapchainKHR Swapchain;
    vk::CommandPool CommandPool;
    vk::Semaphore Semaphore;
};


class VKContext: public Context {
    friend Ultra::VKTest;

public:
    VKContext(void *window);	// previous CreateContext
    virtual ~VKContext();		// previous DestroyContext

    virtual void Load() override;		// previous LoadGL

    virtual void Attach() override;
    virtual void Detach() override;

    // Accessors
    virtual void *GetNativeContext() override;
    virtual bool const IsCurrentContext() override;

    // Mutators
    virtual void SetViewport(uint32_t width, uint32_t height, int32_t x = 0, int32_t y = 0) override;
    virtual void SwapBuffers() override;

    // Settings
    virtual void SetVSync(bool activate) override;


private:
    HWND WindowHandle;

    Reference<VKInstance> mInstance;
    Reference<VKPhysicalDevice> mPhysicalDevice;
    Reference<VKDevice> mDevice;
    vk::SurfaceKHR Surface;
    vk::SurfaceCapabilitiesKHR mCapabilities;
    vk::SurfaceFormatKHR mFormat;
    Reference<VKSwapChain> mSwapChain;
};

}
