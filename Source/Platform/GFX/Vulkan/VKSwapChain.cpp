#include "VKSwapChain.h"

#include "Omnia/Log.h"

namespace Omnia {

uint32_t GetMemoryTypeIndex(const vk::PhysicalDevice &physicalDevice, uint32_t typeBits, vk::MemoryPropertyFlags properties) {
    auto gpuMemoryProps = physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < gpuMemoryProps.memoryTypeCount; i++) {
        if ((typeBits & 1) == 1) {
            if ((gpuMemoryProps.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        typeBits >>= 1;
    }
    return 0;
};

VKSwapChain::VKSwapChain(const vk::Instance &instance, Reference<VKDevice> &device, const vk::SurfaceKHR &surface): mInstance(instance), mDevice(device), mSurface(surface) {
    
    
}

void VKSwapChain::Create(uint32_t width, uint32_t height) {
    const vk::PhysicalDevice &physicalDevice = mDevice->GetPhysicalDevice()->GePhysicalDevice();
    QueueFamilyIndex = mDevice->GetPhysicalDevice()->mQueueFamilyIndices.Graphics;
    vk::Extent2D swapchainSize = vk::Extent2D(width, height);


    // Surface Formats
    vector<vk::SurfaceFormatKHR> surfaceFormats = physicalDevice.getSurfaceFormatsKHR(mSurface);
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
        vk::FormatProperties depthFormatProperties = physicalDevice.getFormatProperties(format);

        // Format must support depth stencil attachment for optimal tiling
        if (depthFormatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            SurfaceDepthFormat = format;
            break;
        }
    }


    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(mSurface);
    if (!(surfaceCapabilities.currentExtent.width == -1 || surfaceCapabilities.currentExtent.height == -1)) {
        swapchainSize = surfaceCapabilities.currentExtent;
        mRenderArea = vk::Rect2D(vk::Offset2D(), swapchainSize);
        mViewport = vk::Viewport(0.0f, 0.0f, static_cast<float>(swapchainSize.width), static_cast<float>(swapchainSize.height), 0, 1.0f);
    }

    // VSync: Disabled
    std::vector<vk::PresentModeKHR> surfacePresentModes = physicalDevice.getSurfacePresentModesKHR(mSurface);
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eImmediate;
    for (vk::PresentModeKHR &mode : surfacePresentModes) {
        if (mode == vk::PresentModeKHR::eMailbox) {
            presentMode = vk::PresentModeKHR::eMailbox;
            break;
        }
    }

    // Create Swapchain
    mDevice->GetDevice().waitIdle();
    vk::SwapchainKHR oldSwapchain = mSwapchain;

    uint32_t imageCount = std::clamp(surfaceCapabilities.maxImageCount, 1U, 3U);
    mRenderArea = vk::Rect2D(vk::Offset2D(), mSurfaceSize);
    mSurfaceSize = vk::Extent2D(std::clamp(width, 1U, 8192U), std::clamp(height, 1U, 8192U));
    mViewport = vk::Viewport(0.0f, 0.0f, static_cast<float>(mSurfaceSize.width), static_cast<float>(mSurfaceSize.height), 0, 1.0f);
    
    vk::SwapchainCreateInfoKHR createInfo = {};
    createInfo.flags = vk::SwapchainCreateFlagsKHR();
    createInfo.surface = mSurface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = SurfaceColorFormat;
    createInfo.imageColorSpace = SurfaceColorSpace;
    createInfo.imageExtent = mSurfaceSize;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    createInfo.imageSharingMode = vk::SharingMode::eExclusive;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &QueueFamilyIndex;
    createInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = oldSwapchain;

    mSwapchain = mDevice->GetDevice().createSwapchainKHR(createInfo);
    if (oldSwapchain != vk::SwapchainKHR(nullptr)) mDevice->GetDevice().destroySwapchainKHR(oldSwapchain);
    mSwapchainBuffers.resize(imageCount);
}

void VKSwapChain::Destroy() {
    mDevice->GetDevice().destroyRenderPass(RenderPass);
    DestroyFrameBuffer();
    mDevice->GetDevice().destroySwapchainKHR(mSwapchain);

    mDevice->GetDevice().destroySemaphore(PresentCompleteSemaphore);
    mDevice->GetDevice().destroySemaphore(RenderCompleteSemaphore);
    for (vk::Fence &fence : WaitFences) {
        mDevice->GetDevice().destroyFence(fence);
    }
}

void VKSwapChain::Resize(uint32_t width, uint32_t height) {
}


void VKSwapChain::CreateRenderPass() {
    vector<vk::AttachmentDescription> attachmentDescriptions = {
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
        vk::AttachmentDescription(
            vk::AttachmentDescriptionFlags(),
            SurfaceDepthFormat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eDontCare,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        )
    };

    vector<vk::AttachmentReference> colorReferences = {
        vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal)
    };

    vector<vk::AttachmentReference> depthReferences = {
        vk::AttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal)
    };

    vector<vk::SubpassDescription> subpasses = {
        vk::SubpassDescription(
            vk::SubpassDescriptionFlags(),
        vk::PipelineBindPoint::eGraphics,
        0,
        nullptr,
        static_cast<uint32_t>(colorReferences.size()),
        colorReferences.data(),
        nullptr,
        depthReferences.data(),
        0,
        nullptr
        )
    };

    vector<vk::SubpassDependency> dependencies = {
        vk::SubpassDependency(
            ~0U,
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

    RenderPass = mDevice->GetDevice().createRenderPass(
        vk::RenderPassCreateInfo(
        vk::RenderPassCreateFlags(),
        static_cast<uint32_t>(attachmentDescriptions.size()),
        attachmentDescriptions.data(),
        static_cast<uint32_t>(subpasses.size()),
        subpasses.data(),
        static_cast<uint32_t>(dependencies.size()),
        dependencies.data()
    )
    );
}

void VKSwapChain::CreateSynchronization() {
    // Semaphore used to ensures that image presentation is complete before starting to submit again
    PresentCompleteSemaphore = mDevice->GetDevice().createSemaphore(vk::SemaphoreCreateInfo());

    // Semaphore used to ensures that all commands submitted have been finished before submitting the image to the queue
    RenderCompleteSemaphore = mDevice->GetDevice().createSemaphore(vk::SemaphoreCreateInfo());

    // Fence for command buffer completion
    WaitFences.resize(GetSwapchainBuffer().size());

    for (size_t i = 0; i < WaitFences.size(); i++) {
        WaitFences[i] = mDevice->GetDevice().createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }
}


void VKSwapChain::LoadFrameBuffer() {
    // Create Depth Image
    DepthImage = mDevice->GetDevice().createImage(vk::ImageCreateInfo(
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
    ));

    vk::MemoryRequirements depthMemoryReq = mDevice->GetDevice().getImageMemoryRequirements(DepthImage);

    // Search through GPU memory properies to see if this can be device local.
    DepthImageMemory = mDevice->GetDevice().allocateMemory(vk::MemoryAllocateInfo(
        depthMemoryReq.size,
        GetMemoryTypeIndex(mDevice->GetPhysicalDevice()->GePhysicalDevice(), depthMemoryReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
    )
    );

    mDevice->GetDevice().bindImageMemory(
        DepthImage,
        DepthImageMemory,
        0
    );

    vk::ImageView depthImageView = mDevice->GetDevice().createImageView(
        vk::ImageViewCreateInfo(
        vk::ImageViewCreateFlags(),
        DepthImage,
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

    std::vector<vk::Image> swapchainImages = mDevice->GetDevice().getSwapchainImagesKHR(mSwapchain);

    for (size_t i = 0; i < swapchainImages.size(); i++)
    {
        GetSwapchainBuffer()[i].image = swapchainImages[i];

        // Color
        GetSwapchainBuffer()[i].views[0] =
            mDevice->GetDevice().createImageView(
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
        GetSwapchainBuffer()[i].views[1] = depthImageView;

       GetSwapchainBuffer()[i].frameBuffer = mDevice->GetDevice().createFramebuffer(
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

void VKSwapChain::DestroyFrameBuffer() {
    mDevice->GetDevice().freeMemory(DepthImageMemory);
    mDevice->GetDevice().destroyImage(DepthImage);
    if (!GetSwapchainBuffer().empty()) {
        mDevice->GetDevice().destroyImageView(GetSwapchainBuffer()[0].views[1]);
    }
    for (size_t i = 0; i < GetSwapchainBuffer().size(); i++) {
        mDevice->GetDevice().destroyImageView(GetSwapchainBuffer()[i].views[0]);
        mDevice->GetDevice().destroyFramebuffer(GetSwapchainBuffer()[i].frameBuffer);
    }
}


}
