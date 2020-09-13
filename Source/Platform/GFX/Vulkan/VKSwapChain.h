#pragma once

#include "Omnia/Omnia.pch"
#include "Omnia/Core.h"

#include "Vulkan.h"

#include "VKDevice.h"


namespace Omnia {

struct SwapChainBuffer {
    vk::Image image;
    std::array<vk::ImageView, 2> views;
    vk::Framebuffer frameBuffer;
};

struct VKSemaphores {
    vk::Semaphore PresentComplete;
    vk::Semaphore RenderComplete;
};

struct VKSwapChainBuffer {
    vk::Image Image;
    vk::ImageView View;
};

struct VKDepthStencil {
    vk::DeviceMemory Memory;
    vk::Image Image;
    vk::ImageView View;
};

class VKSwapChain {
    friend class VKContext; // ToDo: Only needed while porting code

public:
    VKSwapChain(const vk::Instance &instance, Reference<VKDevice> &device, const vk::SurfaceKHR &surface);
    ~VKSwapChain() = default;

    void Create(uint32_t width, uint32_t height);
    void Resize(uint32_t width, uint32_t height);

    void CreateRenderPass();
    void LoadFrameBuffer();
    void DestroyFrameBuffer();

    
private:
    vk::Instance mInstance = nullptr;
    Reference<VKDevice> mDevice = nullptr;
    vk::SurfaceKHR mSurface = nullptr;
    
    vk::SwapchainKHR mSwapchain = nullptr;

    vk::RenderPass RenderPass;

    vk::Extent2D mSurfaceSize;
    vk::Rect2D mRenderArea;
    vk::Viewport mViewport;

    // Resources
    vk::Format SurfaceColorFormat;
    vk::ColorSpaceKHR SurfaceColorSpace;
    vk::Format SurfaceDepthFormat;
    vk::Image DepthImage;
    vk::DeviceMemory DepthImageMemory;

    // Swpachain
    vector<SwapChainBuffer> mSwapchainBuffers;

    uint32_t QueueFamilyIndex = 0;


public:

    vector<SwapChainBuffer> &GetSwapchainBuffer() {
        return mSwapchainBuffers;
    }
};

}
