#pragma once

#include "Omnia/Omnia.pch"
#include "Omnia/Core.h"

#include "Vulkan.h"

#include "VKAllocator.h"
#include "VKDevice.h"

namespace Omnia {

struct VKSwapChainBuffer {
    vk::Framebuffer FrameBuffer;
    vk::Image Image;
    vk::ImageView View;
    array<vk::ImageView, 2> Views;
};

struct VKDepthStencilBuffer {
    vk::DeviceMemory Memory;
    vk::Image Image;
    vk::ImageView View;
};

struct VKSurfaceProperties {
    vk::ClearValue ClearValues[2];
    bool SynchronizedDraw = false;

    vk::Extent2D Size;

    vk::Viewport Viewport;
    vk::Rect2D RenderArea;

    vk::Format ColorFormat;
    vk::ColorSpaceKHR ColorSpace;
    vk::Format DepthFormat;
};

struct VKSynchronization {
    const uint64_t DefaultTimeout = 5000000;
    size_t Timeout = 5000000;

    vector<vk::Semaphore> PresentComplete;
    vector<vk::Semaphore> RenderComplete;

    vector<vk::Fence> WaitFences;
};

class VKSwapChain {
public:
    // Default
    VKSwapChain(const Reference<VKDevice> &device, const vk::SurfaceKHR &surface);
    ~VKSwapChain();

    void Create(uint32_t width, uint32_t height, bool synchronizedDraw = false);
    void Destroy();
    void Resize(uint32_t width, uint32_t height);
    void Prepare();
    void Finish();

    // Accessors
    const vk::CommandBuffer &GetCurrentDrawCommandBuffer() const;
    const vk::Framebuffer &GetCurrentFramebuffer() const;
    const uint32_t GetImageCount() const;
    const vk::Pipeline &GetPipeline() const;
    const vk::Rect2D &GetRenderArea() const;
    const vk::RenderPass &GetRenderPass() const;

    // Mutators
    void SetSyncronizedDraw(bool enable);

    // Extension RenderPass
    vk::CommandBuffer PrepareRenderPass();
    void FinishRenderPass();

private:
    // Helpers
    void CreateImageViews();
    void CreateDepthStencilImageViews();
    void CreateRenderPass();
    void CreatePipeline();
    void CreateFrameBuffer();
    void CreateDrawCommandBuffers();

    // Internal
    void ChooseCapabilities(const vk::SurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height);
    void ChooseSurfaceFormat(const vector<vk::SurfaceFormatKHR> &surfaceFormats);
    void ChoosePresentModes(const vector<vk::PresentModeKHR> &presentModes, bool sync);
    vk::Result QueuePresent(uint32_t imageIndex, vk::Semaphore renderComplete = nullptr);

private:
    VKAllocator mAllocator;
    Reference<VKDevice> mDevice = nullptr;

    uint32_t CurrentFrame = 0;
    uint32_t CurrentBufferIndex = 0;
    uint32_t mImageCount = 0;
    uint32_t QueueFamilyIndex = 0; // ToDo:: Remove

    // Surface
    vk::SurfaceKHR mSurface = nullptr;
    VKSurfaceProperties mSurfaceProperties;

    // SwapChain
    vk::SwapchainKHR mSwapchain = nullptr;
    vk::PresentModeKHR mPresentMode;
    vector<VKSwapChainBuffer> mSwapchainBuffers;
    VKDepthStencilBuffer mColorAttachment;
    VKDepthStencilBuffer mDepthStencil;
    vector<vk::CommandBuffer> mDrawCommandBuffers;
    VKSynchronization mSynchronization;
    vk::DescriptorImageInfo mDescriptorImageInfo;
    vk::Sampler mColorAttachmentSampler;

    // ToDo: Move RenderPass & Pipeline
    vk::RenderPass mRenderPass;
    vk::Pipeline mPipeline;
    vk::PipelineLayout mPipelineLayout;
};

}
