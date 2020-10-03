#pragma once

#include "Omnia/Omnia.pch"
#include "Omnia/Core.h"

#include "Vulkan.h"

#include "VKAllocator.h"
#include "VKDevice.h"

namespace Ultra {
class VKTest;
}


namespace Omnia {

struct SwapChainBuffer {
    vk::Framebuffer frameBuffer;
    vk::Image image;
    std::array<vk::ImageView, 2> views;
};

struct VKSemaphores {
    vector<vk::Semaphore> PresentComplete;
    vector<vk::Semaphore> RenderComplete;
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
    friend Ultra::VKTest;

public:
    VKSwapChain(const Reference<VKDevice> &device, const vk::SurfaceKHR &surface);
    ~VKSwapChain();

    void Load();
    void Create(uint32_t width, uint32_t height, bool vsync = false);
    void Destroy();

    void Cleanup();
    void Resize(uint32_t width, uint32_t height);
    void Prepare();
    void Present();

    const uint32_t GetImageCount() const;
    const uint32_t GetCurrentBufferIndex() const;
    const vk::CommandBuffer &GetCurrentDrawCommandFramebuffer();
    const vk::Framebuffer &GetCurrentFramebuffer();
    const vk::CommandBuffer &GetDrawCommandBuffer(size_t index);
    const vk::Framebuffer &GetFramebuffer(size_t index);
    const vk::RenderPass &GetRenderPass();
    const vk::Rect2D &GetRenderArea() { return mRenderArea; }
    
    // ToDo: Remove after testing
    void Test();
    vk::CommandPool &GetCommandPool() { return mCommandPool; }
    vector<vk::CommandBuffer> &GetCommandBuffers() { return mDrawCommandBuffers; }
    // ~ToDo

public:
    vk::Result AquireNextImage(vk::Semaphore presentComplete, uint32_t *index);
    vk::Result QueuePresent(vk::Queue queue, uint32_t imageIndex, vk::Semaphore wait = nullptr);
private:
    void FindImageFormatAndColorSpace();

    void CreateImageViews();
    void CreateDepthStencilBuffer();
    void CreateRenderPass();
    void CreatePipeline();
    void CreateFrameBuffer();
    void CreateCommandBuffers();
    void CreateDrawBuffers();
    void CreateSynchronization();

    void DestroyFrameBuffer();

    void ChooseCapabilities(const vk::SurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height);
    void ChooseSurfaceFormat(const vector<vk::SurfaceFormatKHR> &surfaceFormats);
    void ChoosePresentModes(const vector<vk::PresentModeKHR> &presentModes, bool sync = false);

private:
    VKAllocator mAllocator;
    Reference<VKDevice> mDevice = nullptr;
    uint32_t CurrentFrame = 0;
    uint32_t CurrentBufferIndex = 0;
    uint32_t QueueFamilyIndex = 0; // ToDo:: Remove

    // Attachments
    vk::Format SurfaceColorFormat;
    vk::Format SurfaceDepthFormat;
    vk::ColorSpaceKHR SurfaceColorSpace;
    VKDepthStencil mDepthStencil;
    vector<vk::CommandBuffer> mDrawCommandBuffers;
    vector<vk::Framebuffer> mFramebuffers;

    // SwapChain
    vk::SwapchainKHR mSwapchain = nullptr;
    vk::PresentModeKHR mPresentMode;
    uint32_t mImageCount = 0;
    vector<vk::Image> mImages;
    vector<VKSwapChainBuffer> mSwapchainBuffers;
    vector<SwapChainBuffer> mSwapchainBuffers2;

    // RenderPass
    vk::CommandPool mCommandPool;
    vk::RenderPass RenderPass;

    // Pipeline
    vk::Pipeline mPipeline;
    vk::PipelineLayout mPipelineLayout;

    // Surface
    vk::SurfaceKHR mSurface = nullptr;
    vk::Rect2D mRenderArea;
    vk::Extent2D mSurfaceSize;
    vk::Extent2D mSurfaceScissor;
    vk::Viewport mViewport;

    // Synchronisation
    VKSemaphores mSemaphores;
    vector<vk::Fence> mWaitFences;
};

}
