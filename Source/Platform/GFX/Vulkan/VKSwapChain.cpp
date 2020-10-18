#include "VKSwapChain.h"

#include "Omnia/Log.h"
#include "Omnia/System/FileSystem.h"

#include <Omnia/UI/GuiBuilder.h>

namespace Omnia {

// Default
VKSwapChain::VKSwapChain(const Reference<VKDevice> &device, const vk::SurfaceKHR &surface): mDevice(device), mSurface(surface) {
    // Verify if the surface is supported by the selected physical device
    if (!mDevice->GetPhysicalDevice()->Call().getSurfaceSupportKHR(GraphicsQueueIndex, mSurface)) {
        AppLogCritical("[GFX::Context::SwapChain] ", "The requested surface isn't supported on the selected physical device!");
    }

    ComputeQueueIndex = mDevice->GetPhysicalDevice()->GetQueueFamilyIndex(vk::QueueFlagBits::eCompute);
    GraphicsQueueIndex = mDevice->GetPhysicalDevice()->GetQueueFamilyIndex(vk::QueueFlagBits::eGraphics);

    mAllocator = { mDevice, "SwapChain" };
}

VKSwapChain::~VKSwapChain() {
    Destroy();
}

void VKSwapChain::Create(uint32_t width, uint32_t height, bool synchronizedDraw) {
    // Check SwapChain support details (capabilities, formats, present modes)
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = mDevice->GetPhysicalDevice()->Call().getSurfaceCapabilitiesKHR(mSurface);
    ChooseCapabilities(surfaceCapabilities, width, height);
    vector<vk::SurfaceFormatKHR> surfaceFormats = mDevice->GetPhysicalDevice()->Call().getSurfaceFormatsKHR(mSurface);
    ChooseSurfaceFormat(surfaceFormats);
    vector<vk::PresentModeKHR> surfacePresentModes = mDevice->GetPhysicalDevice()->Call().getSurfacePresentModesKHR(mSurface);
    ChoosePresentModes(surfacePresentModes, synchronizedDraw);

    // Swapchain Properties
    mDevice->Call().waitIdle();
    vk::SwapchainKHR oldSwapchain = mSwapchain;
    mImageCount = std::clamp(surfaceCapabilities.maxImageCount, surfaceCapabilities.minImageCount, mMaxImageCount);
    mSurfaceProperties.ClearValues[0].color = array<float, 4> { 0.0f, 0.0f, 0.0f, 0.72f};
    mSurfaceProperties.ClearValues[1].depthStencil = { 1.0f, 0 };

    // Get the transformation of the surface
    vk::SurfaceTransformFlagBitsKHR surfaceTransform;
    if (surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) {
        surfaceTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    } else {
        surfaceTransform = surfaceCapabilities.currentTransform;
    }

    // Select a supported composite alpha format (not all devices support alpha opaque)
    vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    vector<vk::CompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
        vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
        vk::CompositeAlphaFlagBitsKHR::eInherit,
    };
    for (auto &flag : compositeAlphaFlags) {
        if (surfaceCapabilities.supportedCompositeAlpha & flag) {
            compositeAlpha = flag;
            break;
        }
    }

    // Create SwapChain
    vk::SwapchainCreateInfoKHR createInfo = {};
    createInfo.flags = vk::SwapchainCreateFlagsKHR();
    createInfo.surface = mSurface;
    createInfo.minImageCount = mImageCount;
    createInfo.imageFormat = mSurfaceProperties.ColorFormat;
    createInfo.imageColorSpace = mSurfaceProperties.ColorSpace;
    createInfo.imageExtent = mSurfaceProperties.Size;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    createInfo.imageArrayLayers = 1;
    createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    createInfo.queueFamilyIndexCount = 0; // ToDo: Look after presentFamily!
    createInfo.preTransform = surfaceTransform;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.compositeAlpha = compositeAlpha;
    createInfo.presentMode = mPresentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapchain;
    if (surfaceCapabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferSrc) {
        createInfo.imageUsage |= vk::ImageUsageFlagBits::eTransferSrc;
    }
    if (surfaceCapabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst) {
        createInfo.imageUsage |= vk::ImageUsageFlagBits::eTransferDst;
    }
    try {
        mSwapchain = mDevice->Call().createSwapchainKHR(createInfo);
    } catch (vk::SystemError exception) {
        AppLogCritical(exception.what());
    }

    // Destroy previous SwapChain and ImageViews
    if (oldSwapchain != vk::SwapchainKHR(nullptr)) {
        mDevice->Call().destroySwapchainKHR(oldSwapchain);
    }
    
    CreateImageViews();
    CreateRenderPass();
    CreateFrameBuffer();
    CreateCommandBuffers();

    // Create Fences and Semaphores
    mSynchronization.PresentComplete.resize(mSwapchainBuffers.size());
    mSynchronization.RenderComplete.resize(mSwapchainBuffers.size());
    mSynchronization.WaitFences.resize(mSwapchainBuffers.size());

    for (size_t i = 0; i < mSwapchainBuffers.size(); i++) {
        // Semaphore used to ensures that image presentation is complete before starting to submit again
        mSynchronization.PresentComplete[i] = mDevice->Call().createSemaphore(vk::SemaphoreCreateInfo());

        // Semaphore used to ensures that all commands submitted have been finished before submitting the image to the queue
        mSynchronization.RenderComplete[i] = mDevice->Call().createSemaphore(vk::SemaphoreCreateInfo());

        // Fence for command buffer completion
        mSynchronization.WaitFences[i] = mDevice->Call().createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }
}

void VKSwapChain::Destroy() {
    mDevice->Call().waitIdle();

    for (vk::Fence &fence : mSynchronization.WaitFences) {
        mDevice->Call().destroyFence(fence);
    }
    for (vk::Semaphore &semaphore : mSynchronization.RenderComplete) {
        mDevice->Call().destroySemaphore(semaphore);
    }
    for (vk::Semaphore &semaphore : mSynchronization.PresentComplete) {
        mDevice->Call().destroySemaphore(semaphore);
    }

    mDevice->Call().freeCommandBuffers(mCommandPool, mUICommandBuffers);
    mDevice->Call().freeCommandBuffers(mCommandPool, mDrawCommandBuffers);
    mDevice->Call().destroyCommandPool(mCommandPool);

    mDevice->Call().destroyRenderPass(mRenderPass);

    if (mSwapchain) {
        mDevice->Call().destroyImageView(mDepthStencil.View);
        mDevice->Call().destroyImage(mDepthStencil.Image);
        mDevice->Call().freeMemory(mDepthStencil.Memory);

        for (auto &&buffer : mSwapchainBuffers) {
            mDevice->Call().destroyImageView(buffer.View);
            mDevice->Call().destroyFramebuffer(buffer.FrameBuffer, nullptr);
        }

        mDevice->Call().destroySwapchainKHR(mSwapchain);
        mSwapchain = nullptr;
    }
}

void VKSwapChain::Resize(uint32_t width, uint32_t height) {
    if(!mSwapchain) return;
    mDevice->Call().waitIdle();
    for (vk::Fence &fence : mSynchronization.WaitFences) {
        mDevice->Call().destroyFence(fence);
    }
    for (vk::Semaphore &semaphore : mSynchronization.RenderComplete) {
        mDevice->Call().destroySemaphore(semaphore);
    }
    for (vk::Semaphore &semaphore : mSynchronization.PresentComplete) {
        mDevice->Call().destroySemaphore(semaphore);
    }

    mDevice->Call().freeCommandBuffers(mCommandPool, mUICommandBuffers);
    mDevice->Call().freeCommandBuffers(mCommandPool, mDrawCommandBuffers);
    mDevice->Call().resetCommandPool(mCommandPool, vk::CommandPoolResetFlagBits::eReleaseResources);

    mDevice->Call().destroyImageView(mDepthStencil.View);
    mDevice->Call().destroyImage(mDepthStencil.Image);
    mDevice->Call().freeMemory(mDepthStencil.Memory);

    for (auto &&buffer : mSwapchainBuffers) {
        mDevice->Call().destroyImageView(buffer.View);
        mDevice->Call().destroyFramebuffer(buffer.FrameBuffer);
    }

    Create(width, height, mSurfaceProperties.SynchronizedDraw); // ToDo:: Causes currently a VRAM memory leak!
}

void VKSwapChain::Prepare() {
    CurrentBufferIndex = mDevice->Call().acquireNextImageKHR(mSwapchain, UINT64_MAX, mSynchronization.PresentComplete[CurrentFrame], nullptr).value;
}

vk::CommandBuffer VKSwapChain::PrepareUI() {
    vk::CommandBuffer drawCommandBuffer = GetCurrentDrawCommandBuffer();
    drawCommandBuffer.begin(vk::CommandBufferBeginInfo());

    vk::RenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.renderPass = mRenderPass;
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.framebuffer = GetCurrentFramebuffer();
    renderPassInfo.renderArea.extent = mSurfaceProperties.Size;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = mSurfaceProperties.ClearValues;

    // Primary CommandBuffer
    {
        drawCommandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline); 

        drawCommandBuffer.setViewport(0, mSurfaceProperties.Viewport);
        drawCommandBuffer.setScissor(0, mSurfaceProperties.RenderArea);

        drawCommandBuffer.endRenderPass();
    }

    // ToDo: Memory leak !!!
    vk::CommandBuffer uiCommandBuffer = mUICommandBuffers[CurrentBufferIndex];
    {
        drawCommandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eSecondaryCommandBuffers);

        vk::CommandBufferInheritanceInfo inheritanceInfo = {};
        inheritanceInfo.renderPass = mRenderPass;
        inheritanceInfo.framebuffer = GetCurrentFramebuffer();

        vk::CommandBufferBeginInfo bufferInfo = {};
        bufferInfo.flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;
        bufferInfo.pInheritanceInfo = &inheritanceInfo;

        uiCommandBuffer.begin(bufferInfo);

        uiCommandBuffer.setViewport(0, mSurfaceProperties.Viewport);
        uiCommandBuffer.setScissor(0, mSurfaceProperties.RenderArea);

        // The Commands here are currently external
    }

    return uiCommandBuffer;
}

void VKSwapChain::Finish() {
    mDevice->Call().waitForFences(1, &mSynchronization.WaitFences[CurrentFrame], VK_TRUE, mSynchronization.Timeout);
    mDevice->Call().resetFences(mSynchronization.WaitFences[CurrentFrame]);

    vk::Semaphore signalSemaphores[] = { mSynchronization.RenderComplete[CurrentBufferIndex] };
    vk::Semaphore waitSemaphores[] = { mSynchronization.PresentComplete[CurrentFrame] };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    vk::SubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pCommandBuffers = &mDrawCommandBuffers[CurrentBufferIndex];
    submitInfo.pSignalSemaphores = signalSemaphores;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    mDevice->GetQueue().submit(submitInfo, mSynchronization.WaitFences[CurrentFrame]);

    mDevice->Call().waitForFences(1, &mSynchronization.WaitFences[CurrentFrame], VK_TRUE, mSynchronization.Timeout);
    mDevice->Call().resetFences(mSynchronization.WaitFences[CurrentFrame]);

    vk::Result result = QueuePresent(CurrentBufferIndex, mSynchronization.RenderComplete[CurrentBufferIndex]);
    // Swapchain lost, we'll try again next poll
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        Resize(mSurfaceProperties.Size.width, mSurfaceProperties.Size.height);
        return;
    } else if (result == vk::Result::eErrorDeviceLost) {
        AppLogCritical("Device Lost");
    }

    CurrentFrame = (CurrentFrame + 1) % mImageCount;

    mDrawCommandBuffers[CurrentBufferIndex].reset(vk::CommandBufferResetFlagBits::eReleaseResources);
}

void VKSwapChain::FinishUI() {
    vk::CommandBuffer drawCommandBuffer = GetCurrentDrawCommandBuffer();
    vk::CommandBuffer uiCommandBuffer = mUICommandBuffers[CurrentBufferIndex];

    uiCommandBuffer.end();
    drawCommandBuffer.executeCommands(uiCommandBuffer);;
    drawCommandBuffer.endRenderPass();

    drawCommandBuffer.end();
}

// Helpers
void VKSwapChain::CreateImageViews() {
    // SwapChain Color Attachment
    /// Images
    vector<vk::Image> images;
    mDevice->Call().getSwapchainImagesKHR(mSwapchain, &mImageCount, nullptr);
    images.resize(mImageCount);
    mDevice->Call().getSwapchainImagesKHR(mSwapchain, &mImageCount, images.data());

    /// Views
    mSwapchainBuffers.resize(mImageCount);
    for (size_t i = 0; i < mImageCount; i++) {
        vk::ImageViewCreateInfo createInfo = {};
        createInfo.format = mSurfaceProperties.ColorFormat;
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.components = {       // eIdentity for all?
            vk::ComponentSwizzle::eR,
            vk::ComponentSwizzle::eG,
            vk::ComponentSwizzle::eB,
            vk::ComponentSwizzle::eA,
        };
        createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        mSwapchainBuffers[i].Image = images[i];
        createInfo.image = mSwapchainBuffers[i].Image;

        mSwapchainBuffers[i].View = mDevice->Call().createImageView(createInfo);
    }

    // SwapChain Depth\Stencil Attachment
    /// Image
    vk::ImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.imageType = vk::ImageType::e2D;
    imageCreateInfo.format = mSurfaceProperties.DepthFormat;
    imageCreateInfo.extent = vk::Extent3D(mSurfaceProperties.Size, 1U);
    imageCreateInfo.mipLevels = 1U;
    imageCreateInfo.arrayLayers = 1U;
    imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
    imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
    imageCreateInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc;
    imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imageCreateInfo.queueFamilyIndexCount = 1;
    imageCreateInfo.pQueueFamilyIndices = &GraphicsQueueIndex;
    imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    mDepthStencil.Image = mDevice->Call().createImage(imageCreateInfo);

    /// Memory (Search through GPU memory properies to see if this can be device local)
    vk::MemoryRequirements memoryRequirements = mDevice->Call().getImageMemoryRequirements(mDepthStencil.Image);
    mAllocator.Allocate(memoryRequirements, &mDepthStencil.Memory, vk::MemoryPropertyFlagBits::eDeviceLocal);
    mDevice->Call().bindImageMemory(mDepthStencil.Image, mDepthStencil.Memory, 0);

    /// View
    vk::ImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.image =  mDepthStencil.Image;
    viewCreateInfo.viewType = vk::ImageViewType::e2D;
    viewCreateInfo.format = mSurfaceProperties.DepthFormat;
    viewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    if (mSurfaceProperties.DepthFormat >= vk::Format::eD16UnormS8Uint) {
        viewCreateInfo.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
    }
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;
    mDepthStencil.View = mDevice->Call().createImageView(viewCreateInfo);
}

void VKSwapChain::CreateRenderPass() {
    // Attachments
    vector<vk::AttachmentDescription> attachments = {
        // Color Attachment
        vk::AttachmentDescription(
            vk::AttachmentDescriptionFlags(),
            mSurfaceProperties.ColorFormat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR
        ),
        // Depth\Stencil Attachment
        vk::AttachmentDescription(
            vk::AttachmentDescriptionFlags(),
            mSurfaceProperties.DepthFormat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        )
    };
    vector<vk::AttachmentReference> colorReferences = {
        vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal),
    };
    vk::AttachmentReference depthReferences = {
        vk::AttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal),
    };

    // Subpasses
    vector<vk::SubpassDescription> subpasses = {
        vk::SubpassDescription(
            vk::SubpassDescriptionFlags(),
            vk::PipelineBindPoint::eGraphics,
            nullptr,
            colorReferences,
            nullptr,
            &depthReferences,
            0
        )
    };

    // Dependencies
    vector<vk::SubpassDependency> dependencies = {
        vk::SubpassDependency(
            0,
            VK_SUBPASS_EXTERNAL,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::DependencyFlagBits::eByRegion
        )
        //vk::SubpassDependency(
        //    VK_SUBPASS_EXTERNAL,
        //    0,
        //    vk::PipelineStageFlagBits::eFragmentShader,
        //    vk::PipelineStageFlagBits::eColorAttachmentOutput,
        //    vk::AccessFlagBits::eMemoryRead,
        //    vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
        //    vk::DependencyFlagBits::eByRegion
        //),
        //vk::SubpassDependency(
        //    0,
        //    VK_SUBPASS_EXTERNAL,
        //    vk::PipelineStageFlagBits::eColorAttachmentOutput,
        //    vk::PipelineStageFlagBits::eFragmentShader,
        //    vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
        //    vk::AccessFlagBits::eMemoryRead,
        //    vk::DependencyFlagBits::eByRegion
        //)
    };

    // Finally
    mRenderPass = mDevice->Call().createRenderPass(
        vk::RenderPassCreateInfo(
            vk::RenderPassCreateFlags(),
            attachments,
            subpasses,
            dependencies
        )
    );
}

void VKSwapChain::CreateFrameBuffer() {
    vector<vk::ImageView> attachments;
    attachments.resize(2);

    vk::FramebufferCreateInfo createInfo = {};
    createInfo.renderPass = mRenderPass;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.width = mSurfaceProperties.Size.width;
    createInfo.height = mSurfaceProperties.Size.height;
    createInfo.layers = 1;

    for (size_t i = 0; i < mSwapchainBuffers.size(); i++) {
        attachments[0] = mSwapchainBuffers[i].View;
        attachments[1] = mDepthStencil.View;

        mSwapchainBuffers[i].FrameBuffer = mDevice->Call().createFramebuffer(createInfo);
    }
}

void VKSwapChain::CreateCommandBuffers() {
    mDrawCommandBuffers.resize(mImageCount);

    vk::CommandPoolCreateInfo createInfo = {};
    createInfo.queueFamilyIndex = GraphicsQueueIndex;
    createInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;
    mCommandPool = mDevice->Call().createCommandPool(createInfo);

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.commandPool = mCommandPool;
    commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
    commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(mDrawCommandBuffers.size());
    mDrawCommandBuffers = mDevice->Call().allocateCommandBuffers(commandBufferAllocateInfo);

    for (size_t i = 0; i < mDrawCommandBuffers.size(); i++) {
        mDrawCommandBuffers[i].begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eSimultaneousUse));
        mDrawCommandBuffers[i].end();
    }

    mUICommandBuffers.resize(mImageCount);

    vk::CommandBufferAllocateInfo bufferAllcoateInfo = {};
    bufferAllcoateInfo.commandPool = mCommandPool;
    bufferAllcoateInfo.level = vk::CommandBufferLevel::eSecondary;
    bufferAllcoateInfo.commandBufferCount = mImageCount;

    mUICommandBuffers = mDevice->Call().allocateCommandBuffers(bufferAllcoateInfo);
}

// Accessors
const vk::CommandBuffer &VKSwapChain::GetCurrentDrawCommandBuffer() const {
    return mDrawCommandBuffers[CurrentBufferIndex];
}

const vk::Framebuffer &VKSwapChain::GetCurrentFramebuffer() const {
    return mSwapchainBuffers[CurrentBufferIndex].FrameBuffer;
}

const uint32_t VKSwapChain::GetImageCount() const {
    return mImageCount;
}

const vk::Rect2D & VKSwapChain::GetRenderArea() const {
    return mSurfaceProperties.RenderArea;
}

const vk::RenderPass &VKSwapChain::GetRenderPass() const {
    return mRenderPass;
}

/// @brief Test
intptr_t *VKSwapChain::GetAttachmentID() {
    static ImTextureID viewportID = 0;

    static bool test = false;
    if (!test) {
        array<vk::AttachmentDescription, 2> attachmentDescriptions;
        struct FramebufferAttachment {
            vk::Image Image;
            vk::DeviceMemory Memory;
            vk::ImageView View;
        };
        FramebufferAttachment ColorAttachment;
        vk::Sampler ColorAttachmentSampler;
        VkDescriptorImageInfo DescriptorImageInfo;
        const vk::Format COLOR_BUFFER_FORMAT = vk::Format::eR8G8B8A8Unorm;

        vk::ImageCreateInfo imageInfo = {};
        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.format = COLOR_BUFFER_FORMAT;
        imageInfo.extent.width = 680;
        imageInfo.extent.height = 480;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.tiling = vk::ImageTiling::eOptimal;
        imageInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
        ColorAttachment.Image = mDevice->Call().createImage(imageInfo, nullptr);

        vk::MemoryRequirements memoryRequirements;
        memoryRequirements = mDevice->Call().getImageMemoryRequirements(ColorAttachment.Image);

        mAllocator.Allocate(memoryRequirements, &ColorAttachment.Memory);
        mDevice->Call().bindImageMemory(ColorAttachment.Image, ColorAttachment.Memory, 0);

        vk::ImageViewCreateInfo viewInfo = {};
        viewInfo.viewType = vk::ImageViewType::e2D;
        viewInfo.format = COLOR_BUFFER_FORMAT;
        viewInfo.subresourceRange = {};
        viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        viewInfo.image = ColorAttachment.Image;
        ColorAttachment.View = mDevice->Call().createImageView(viewInfo, nullptr);

        vk::SamplerCreateInfo samplerInfo = {};
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.magFilter = vk::Filter::eLinear;
        samplerInfo.minFilter = vk::Filter::eLinear;
        samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
        samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 1.0f;
        samplerInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
        ColorAttachmentSampler = mDevice->Call().createSampler(samplerInfo, nullptr);

        attachmentDescriptions[0].flags = vk::AttachmentDescriptionFlagBits::eMayAlias;
        attachmentDescriptions[0].format = COLOR_BUFFER_FORMAT;
        attachmentDescriptions[0].samples = vk::SampleCountFlagBits::e1;
        attachmentDescriptions[0].loadOp = vk::AttachmentLoadOp::eClear;
        attachmentDescriptions[0].storeOp = vk::AttachmentStoreOp::eStore;
        attachmentDescriptions[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachmentDescriptions[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachmentDescriptions[0].initialLayout = vk::ImageLayout::eUndefined;
        attachmentDescriptions[0].finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

        vk::AttachmentReference colorReference = { 0, vk::ImageLayout::eColorAttachmentOptimal };
        vk::AttachmentReference depthReference = { 1, vk::ImageLayout::eDepthStencilAttachmentOptimal };

        vk::SubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments = &colorReference;
        subpassDescription.pDepthStencilAttachment = &depthReference;

        /// Create the renderpass
        std::array<vk::SubpassDependency, 2> dependencies;
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eFragmentShader;
        dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[0].srcAccessMask = vk::AccessFlagBits::eShaderRead;
        dependencies[0].dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        dependencies[0].dependencyFlags = vk::DependencyFlagBits::eByRegion;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eFragmentShader;
        dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dependencies[1].srcAccessMask = vk::AccessFlagBits::eShaderRead;
        dependencies[1].dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        dependencies[1].dependencyFlags = vk::DependencyFlagBits::eByRegion;

        vk::RenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
        renderPassInfo.pAttachments = attachmentDescriptions.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpassDescription;
        renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies = dependencies.data();
        vk::RenderPass RenderPass = mDevice->Call().createRenderPass(renderPassInfo, nullptr);

        vk::ImageView attachments[2];
        attachments[0] = ColorAttachment.View;

        vk::FramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.renderPass = RenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = 640;
        framebufferInfo.height = 480;
        framebufferInfo.layers = 1;
        vk::Framebuffer Framebuffer = mDevice->Call().createFramebuffer(framebufferInfo, nullptr);

        DescriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        DescriptorImageInfo.imageView = ColorAttachment.View;
        DescriptorImageInfo.sampler = ColorAttachmentSampler;
        viewportID = ImGui_ImplVulkan_AddTexture(DescriptorImageInfo.sampler, DescriptorImageInfo.imageView, DescriptorImageInfo.imageLayout);

        test = true;
    }
    return (intptr_t *)(viewportID);
}

// Mutators
void VKSwapChain::SetSyncronizedDraw(bool enable) {
    if (mSurfaceProperties.SynchronizedDraw != enable) {
        mSurfaceProperties.SynchronizedDraw = enable;
        Resize(mSurfaceProperties.Size.width, mSurfaceProperties.Size.height);
    }
}

// Internal
void VKSwapChain::ChooseCapabilities(const vk::SurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height) {
    if (!(capabilities.currentExtent.width == -1 || capabilities.currentExtent.height == -1)) {
        mSurfaceProperties.Size = capabilities.currentExtent;
    } else {
        mSurfaceProperties.Size = vk::Extent2D(
            std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        );
    }
    mSurfaceProperties.RenderArea = vk::Rect2D(vk::Offset2D(), mSurfaceProperties.Size);
    mSurfaceProperties.Viewport = vk::Viewport(
        0.0f,
        0.0f,
        static_cast<float>(mSurfaceProperties.Size.width),
        static_cast<float>(mSurfaceProperties.Size.height),
        0.0f,
        1.0f
    );
}

void VKSwapChain::ChooseSurfaceFormat(const vector<vk::SurfaceFormatKHR> &surfaceFormats) {
    // Color Buffer
    if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined) {
        mSurfaceProperties.ColorFormat = vk::Format::eB8G8R8A8Unorm;
        mSurfaceProperties.ColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    } else {
        mSurfaceProperties.ColorFormat = surfaceFormats[0].format;
        mSurfaceProperties.ColorSpace = surfaceFormats[0].colorSpace;

        for (auto &&format : surfaceFormats) {
            if (format.format == vk::Format::eB8G8R8A8Unorm) {
                mSurfaceProperties.ColorFormat = format.format;
                mSurfaceProperties.ColorSpace = format.colorSpace;
            }
        }
    }

    // Depth Buffer
    vector<vk::Format> depthFormats = {
        vk::Format::eD32SfloatS8Uint,
        vk::Format::eD32Sfloat,
        vk::Format::eD24UnormS8Uint,
        vk::Format::eD16UnormS8Uint,
        vk::Format::eD16Unorm
    };
    for (vk::Format &format : depthFormats) {
        vk::FormatProperties depthFormatProperties = mDevice->GetPhysicalDevice()->Call().getFormatProperties(format);

        // Format must support depth stencil attachment for optimal tiling
        if (depthFormatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            mSurfaceProperties.DepthFormat = format;
            break;
        }
    }
}

void VKSwapChain::ChoosePresentModes(const vector<vk::PresentModeKHR> &presentModes, bool sync) {
    mPresentMode = vk::PresentModeKHR::eFifo;

    if (!mSurfaceProperties.SynchronizedDraw) {
        for (auto &mode : presentModes) {
            switch (mode) {
                case vk::PresentModeKHR::eMailbox:      { mPresentMode = vk::PresentModeKHR::eMailbox;   break; }
                case vk::PresentModeKHR::eImmediate:    { mPresentMode = vk::PresentModeKHR::eImmediate; break; }
                default:                                { break;}
            }
            if (mPresentMode != vk::PresentModeKHR::eFifo) break;
        }
    }
}

vk::Result VKSwapChain::QueuePresent(uint32_t imageIndex, vk::Semaphore renderComplete) {
    vk::SwapchainKHR swapChains[] = { mSwapchain };
    vk::Semaphore signalSemaphores[] = { renderComplete };

    vk::PresentInfoKHR presentInfo = {};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional

    if (renderComplete) {
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
    }

    return mDevice->GetQueue().presentKHR(presentInfo);
}

}
