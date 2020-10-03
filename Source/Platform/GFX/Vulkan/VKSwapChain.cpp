#include "VKSwapChain.h"

#include "Omnia/Log.h"

namespace Omnia {

VKSwapChain::VKSwapChain(const Reference<VKDevice> &device, const vk::SurfaceKHR &surface): mDevice(device), mSurface(surface) {}

VKSwapChain::~VKSwapChain() {
    Destroy();
}

void VKSwapChain::Load() {
}

void VKSwapChain::Create(uint32_t width, uint32_t height, bool vsync) {

    QueueFamilyIndex = mDevice->GetPhysicalDevice()->GetQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
    vk::Extent2D swapchainSize = vk::Extent2D(width, height);

    // Surface Support
    bool supported = mDevice->GetPhysicalDevice()->Call().getSurfaceSupportKHR(QueueFamilyIndex, mSurface);
    

    // Surface Formats
    vector<vk::SurfaceFormatKHR> surfaceFormats = mDevice->GetPhysicalDevice()->Call().getSurfaceFormatsKHR(mSurface);
    if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined) {
        SurfaceColorFormat = vk::Format::eB8G8R8A8Unorm;
    } else {
        SurfaceColorFormat = surfaceFormats[0].format;
    }
    SurfaceColorSpace = surfaceFormats[0].colorSpace;
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
            SurfaceDepthFormat = format;
            break;
        }
    }


    // Get cababilities and present modes
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = mDevice->GetPhysicalDevice()->Call().getSurfaceCapabilitiesKHR(mSurface);
    vector<vk::PresentModeKHR> surfacePresentModes = mDevice->GetPhysicalDevice()->Call().getSurfacePresentModesKHR(mSurface);
    if (!(surfaceCapabilities.currentExtent.width == -1 || surfaceCapabilities.currentExtent.height == -1)) {
        swapchainSize = surfaceCapabilities.currentExtent;
        mRenderArea = vk::Rect2D(vk::Offset2D(), swapchainSize);
        mViewport = vk::Viewport(0.0f, 0.0f, static_cast<float>(swapchainSize.width), static_cast<float>(swapchainSize.height), 0, 1.0f);
    } else {
    }
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eImmediate;
    for (auto &mode : surfacePresentModes) {
        if (vsync) {
            if (mode == vk::PresentModeKHR::eFifo) {
                presentMode = vk::PresentModeKHR::eFifo;
                break;
            }
        } else {
            if (mode == vk::PresentModeKHR::eMailbox) {
                presentMode = vk::PresentModeKHR::eMailbox;
                break;
            }
        }
    }


    // Create Swapchain
    mDevice->Call().waitIdle();
    vk::SwapchainKHR oldSwapchain = mSwapchain;

    uint32_t imageCount = std::clamp(surfaceCapabilities.maxImageCount, 1U, 3U);
    mRenderArea = vk::Rect2D(vk::Offset2D(), mSurfaceSize);
    mSurfaceSize = vk::Extent2D(std::clamp(width, 1U, 8192U), std::clamp(height, 1U, 8192U));
    mViewport = vk::Viewport(0.0f, 0.0f, static_cast<float>(mSurfaceSize.width), static_cast<float>(mSurfaceSize.height), 0, 1.0f);
    
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

    vk::SwapchainCreateInfoKHR createInfo = {};
    createInfo.flags = vk::SwapchainCreateFlagsKHR();
    createInfo.surface = mSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = SurfaceColorFormat;
    createInfo.imageColorSpace = SurfaceColorSpace;
    createInfo.imageExtent = mSurfaceSize;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    createInfo.imageArrayLayers = 1;
    createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.preTransform = surfaceTransform;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.compositeAlpha = compositeAlpha;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapchain;
    if (surfaceCapabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferSrc) {
        //createInfo.imageUsage = vk::ImageUsageFlagBits::eTransferSrc;
    }
    if (surfaceCapabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst) {
        //createInfo.imageUsage = vk::ImageUsageFlagBits::eTransferDst;
    }
    mSwapchain = mDevice->Call().createSwapchainKHR(createInfo);

    if (oldSwapchain != vk::SwapchainKHR(nullptr)) {
        for (uint32_t i = 0; i < imageCount; i++) {
            //mDevice->Call().destroyImageView(mSwapchainBuffers[i].View, nullptr);
        }
        mDevice->Call().destroySwapchainKHR(oldSwapchain);
    }

    mImages.resize(mImageCount);
    mDevice->Call().getSwapchainImagesKHR(mSwapchain, &mImageCount, mImages.data());

    mSwapchainBuffers2.resize(imageCount);
    mSwapchainBuffers.resize(mImageCount);
    for (size_t i = 0; i < mImageCount; i++) {
        vk::ImageViewCreateInfo colorAttachmentView = {};
        colorAttachmentView.format = SurfaceColorFormat;
        colorAttachmentView.components = {
            vk::ComponentSwizzle::eR,
            vk::ComponentSwizzle::eG,
            vk::ComponentSwizzle::eB,
            vk::ComponentSwizzle::eA,
        };
        colorAttachmentView.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        colorAttachmentView.subresourceRange.baseMipLevel = 0;
        colorAttachmentView.subresourceRange.levelCount = 1;
        colorAttachmentView.subresourceRange.baseArrayLayer = 0;
        colorAttachmentView.subresourceRange.layerCount = 1;
        colorAttachmentView.viewType = vk::ImageViewType::e2D;

        //mSwapchainBuffers[i].Image = mImages[i];
        //colorAttachmentView.image = mSwapchainBuffers[i].Image;
        //mSwapchainBuffers[i].View = mDevice->Call().createImageView(colorAttachmentView, nullptr);
    }

    CreateDrawBuffers();
    CreateDepthStencilBuffer();
    CreateRenderPass();
    CreateSynchronization();
    CreateFrameBuffer();
}

void VKSwapChain::CreateDrawBuffers() {
    mDrawCommandBuffers.resize(mImageCount);

    vk::CommandPoolCreateInfo commandPoolCreateInfo = {};
    commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;
    commandPoolCreateInfo.queueFamilyIndex = QueueFamilyIndex;
    //mCommandPool = mDevice->Call().createCommandPool(commandPoolCreateInfo, nullptr);
    mCommandPool = mDevice->GetCommandPool();

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.commandPool = mCommandPool;
    commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
    commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(mDrawCommandBuffers.size());
    mDrawCommandBuffers = mDevice->Call().allocateCommandBuffers(commandBufferAllocateInfo);
}

void VKSwapChain::CreateDepthStencilBuffer() {
    // Create Depth Image
    mDepthStencil.Image = mDevice->Call().createImage(
        vk::ImageCreateInfo(
            vk::ImageCreateFlags(),
            vk::ImageType::e2D,
            SurfaceDepthFormat,
            vk::Extent3D(mSurfaceSize.width, mSurfaceSize.height, 1),
            1U,
            1U,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc,
            vk::SharingMode::eExclusive,
            1,
            &QueueFamilyIndex,
            vk::ImageLayout::eUndefined
        )
    );

    vk::MemoryRequirements memoryRequirements = mDevice->Call().getImageMemoryRequirements(mDepthStencil.Image);

    // Search through GPU memory properies to see if this can be device local.
    //mAllocator.Allocate(memoryRequirements, &mDepthStencil.Memory, vk::MemoryPropertyFlagBits::eDeviceLocal);
    mDepthStencil.Memory = mDevice->Call().allocateMemory(
        vk::MemoryAllocateInfo(
            memoryRequirements.size,
            mDevice->GetPhysicalDevice()->GetMemoryTypeIndex(memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
        )
    );
    mDevice->Call().bindImageMemory(mDepthStencil.Image, mDepthStencil.Memory, 0);

    // Stencil aspect should only be set on depth + stencil formats (VK_FORMAT_D16_UNORM_S8_UINT..VK_FORMAT_D32_SFLOAT_S8_UINT
    //if (depthFormat >= VK_FORMAT_D16_UNORM_S8_UINT) createImageViewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    mDepthStencil.View = mDevice->Call().createImageView(
        vk::ImageViewCreateInfo(
            vk::ImageViewCreateFlags(),
            mDepthStencil.Image,
            vk::ImageViewType::e2D,
            SurfaceDepthFormat,
            vk::ComponentMapping(),
            vk::ImageSubresourceRange(
                vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil,
                0,
                1,
                0,
                1
            )
        )
    );

}

void VKSwapChain::CreateRenderPass() {
    // Attachments
    vector<vk::AttachmentDescription> attachments = {
        // Color Attachment
        vk::AttachmentDescription(
            vk::AttachmentDescriptionFlags(),
            SurfaceColorFormat,
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
            SurfaceDepthFormat,
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
            VK_SUBPASS_EXTERNAL,
            0,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::AccessFlagBits::eMemoryRead,
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::DependencyFlagBits::eByRegion
        ),
        vk::SubpassDependency(
            0,
            ~0U,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eBottomOfPipe,
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eMemoryRead,
            vk::DependencyFlagBits::eByRegion
        )
    };

    // Finally
    RenderPass = mDevice->Call().createRenderPass(
        vk::RenderPassCreateInfo(
            vk::RenderPassCreateFlags(),
            attachments,
            subpasses,
            dependencies
        )
    );
}

void VKSwapChain::CreateSynchronization() {
    // Semaphore used to ensures that image presentation is complete before starting to submit again
    mSemaphores.PresentComplete = mDevice->Call().createSemaphore(vk::SemaphoreCreateInfo());

    // Semaphore used to ensures that all commands submitted have been finished before submitting the image to the queue
    mSemaphores.RenderComplete = mDevice->Call().createSemaphore(vk::SemaphoreCreateInfo());

    // Fence for command buffer completion
    mWaitFences.resize(mSwapchainBuffers.size());
    for (size_t i = 0; i < mWaitFences.size(); i++) {
        mWaitFences[i] = mDevice->Call().createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }
}

void VKSwapChain::CreateFrameBuffer() {
    //vector<vk::ImageView> attachments;
    //attachments.resize(3);
    //attachments[1] = mDepthStencil.View;

    //mFramebuffers.resize(mImageCount);
    //for (uint32_t i = 0; i < mFramebuffers.size(); i++) {
    //    attachments[0] = mSwapchainBuffers[i].View;

    //    mFramebuffers[i] = mDevice->Call().createFramebuffer(
    //        vk::FramebufferCreateInfo(
    //            vk::FramebufferCreateFlags(),
    //            RenderPass,
    //            attachments
    //            //1280U,
    //            //1024U,
    //            //1
    //        )
    //    );
    //}
    // Should be enough

    std::vector<vk::Image> swapchainImages = mDevice->Call().getSwapchainImagesKHR(mSwapchain);

    for (size_t i = 0; i < swapchainImages.size(); i++) {
        GetSwapchainBuffer()[i].image = swapchainImages[i];

        // Color
        GetSwapchainBuffer()[i].views[0] = mDevice->Call().createImageView(
            vk::ImageViewCreateInfo(
                vk::ImageViewCreateFlags(),
                swapchainImages[i],
                vk::ImageViewType::e2D,
                SurfaceColorFormat,
                vk::ComponentMapping(),
                vk::ImageSubresourceRange(
                    vk::ImageAspectFlagBits::eColor,
                    0,
                    1,
                    0,
                    1
                )
            )
        );

        // Depth
        GetSwapchainBuffer()[i].views[1] = mDepthStencil.View;

        // Framebuffer
        GetSwapchainBuffer()[i].frameBuffer = mDevice->Call().createFramebuffer(
            vk::FramebufferCreateInfo(
                vk::FramebufferCreateFlags(),
                RenderPass,
                static_cast<uint32_t>(GetSwapchainBuffer()[i].views.size()),
                GetSwapchainBuffer()[i].views.data(),
                mSurfaceSize.width, mSurfaceSize.height,
                1
            )
        );
    }
}

void VKSwapChain::Destroy() {
    mDevice->Call().waitIdle();
    mDevice->Call().freeCommandBuffers(mDevice->GetCommandPool(), mDrawCommandBuffers);
    mDevice->Call().destroyRenderPass(RenderPass);
    mDevice->Call().destroySwapchainKHR(mSwapchain);
    mDevice->Call().destroySemaphore(mSemaphores.PresentComplete);
    mDevice->Call().destroySemaphore(mSemaphores.RenderComplete);
    for (vk::Fence &fence : mWaitFences) {
        mDevice->Call().destroyFence(fence);
    }
    mDevice->Call().waitIdle();
}

void VKSwapChain::Cleanup() {
    if (mSwapchain) {
        for (size_t i = 0; i < mImageCount; i++) {
            mDevice->Call().destroyImageView(mSwapchainBuffers[i].View);
        }
        mDevice->Call().destroySwapchainKHR(mSwapchain);
        mSwapchain = nullptr;
    }
}

void VKSwapChain::Resize(uint32_t width, uint32_t height) {
    mDevice->Call().waitIdle();

    Create(width, height);
    if (!mSwapchainBuffers.empty()) {
        mDevice->Call().destroyImageView(mDepthStencil.View);
    }
    mDevice->Call().destroyImage(mDepthStencil.Image);
    mDevice->Call().freeMemory(mDepthStencil.Memory);
    CreateDepthStencilBuffer();

    for (auto &framebuffer : mFramebuffers) {
        mDevice->Call().destroyFramebuffer(framebuffer, nullptr);
    }
    CreateFrameBuffer();

    mDevice->Call().freeCommandBuffers(mCommandPool, static_cast<uint32_t>(mDrawCommandBuffers.size()), mDrawCommandBuffers.data());
    CreateDrawBuffers();

    mDevice->Call().waitIdle();
}

void VKSwapChain::Prepare() {
    AquireNextImage(mSemaphores.PresentComplete, &CurrentBufferIndex);
}

void VKSwapChain::Present() {
    const uint64_t DefaultFenceTimeout = UINT64_MAX;
    int test = GetCurrentBufferIndex();

    mDevice->Call().waitForFences(1, &mWaitFences[CurrentBufferIndex], VK_TRUE, DefaultFenceTimeout);
    mDevice->Call().resetFences(mWaitFences[CurrentBufferIndex]);

    vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mDrawCommandBuffers[CurrentBufferIndex];
    //submitInfo.signalSemaphoreCount = 1;
    //submitInfo.waitSemaphoreCount = 1;
    //submitInfo.pSignalSemaphores = &mSemaphores.RenderComplete;
    submitInfo.pWaitDstStageMask = &waitStageMask;
    //submitInfo.pWaitSemaphores = &mSemaphores.PresentComplete;

    mDevice->GetQueue().submit(submitInfo, mWaitFences[CurrentBufferIndex]);
    vk::Result result = QueuePresent(mDevice->GetQueue(), CurrentBufferIndex, mSemaphores.RenderComplete);
    if (result != vk::Result::eSuccess || result != vk::Result::eSuboptimalKHR) {
        // Swapchain lost, we'll try again next poll
        if (result != vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
            Resize(mSurfaceSize.width, mSurfaceSize.height);
            return;
        } else {
            AppLog(result);
        }
    }
    if (result == vk::Result::eErrorDeviceLost) exit(1); // driver lost, we'll crash in this case:

    mDevice->Call().waitForFences(1, &mWaitFences[CurrentBufferIndex], VK_TRUE, DefaultFenceTimeout);
    mDevice->Call().resetFences(mWaitFences[CurrentBufferIndex]);
}


const uint32_t VKSwapChain::GetImageCount() const {
    return mImageCount;
}

const uint32_t VKSwapChain::GetCurrentBufferIndex() const {
    return CurrentBufferIndex;
}

const vk::CommandBuffer &VKSwapChain::GetCurrentDrawCommandFramebuffer() {
    return GetDrawCommandBuffer(CurrentBufferIndex);
}

const vk::Framebuffer &VKSwapChain::GetCurrentFramebuffer() {
    return GetFramebuffer(CurrentBufferIndex);
}

const vk::CommandBuffer &VKSwapChain::GetDrawCommandBuffer(size_t index) {
    return mDrawCommandBuffers[index];
}

const vk::Framebuffer &VKSwapChain::GetFramebuffer(size_t index) {
    return mFramebuffers[index];
}

const vk::RenderPass &VKSwapChain::GetRenderPass() {
    return RenderPass;
}


vk::Result VKSwapChain::AquireNextImage(vk::Semaphore presentComplete, uint32_t *index) {
    vk::Result result;
    *index = mDevice->Call().acquireNextImageKHR(mSwapchain, UINT64_MAX, presentComplete, (vk::Fence)nullptr);
    return vk::Result::eSuccess;
}

vk::Result VKSwapChain::QueuePresent(vk::Queue queue, uint32_t imageIndex, vk::Semaphore wait) {
    vk::PresentInfoKHR presentInfo = {};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &mSwapchain;
    presentInfo.pImageIndices = &imageIndex;
    if (wait) {
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &wait;
    }
    mDevice->GetQueue().presentKHR(presentInfo);
    return vk::Result::eSuccess;
}

void VKSwapChain::FindImageFormatAndColorSpace() {
    auto formats = mDevice->GetPhysicalDevice()->Call().getSurfaceFormatsKHR(mSurface);

    if ((formats.size() == 1) && formats[0].format == vk::Format::eUndefined) {
        SurfaceColorFormat = vk::Format::eB8G8R8A8Unorm;
        SurfaceColorSpace = formats[0].colorSpace;
    } else {
        bool result = false;
        for (auto &&format : formats) {
            if (format.format == vk::Format::eB8G8R8A8Unorm) {
                SurfaceColorFormat = format.format;
                SurfaceColorSpace = formats[0].colorSpace;
                result = true;
                break;
            }
        }

        if (!result) {
            SurfaceColorFormat = formats[0].format;
            SurfaceColorSpace = formats[0].colorSpace;
        }
    }
}

}
