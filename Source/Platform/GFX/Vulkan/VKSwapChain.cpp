#include "VKSwapChain.h"

#include "Omnia/Log.h"
#include "Omnia/System/FileSystem.h"

namespace Omnia {

VKSwapChain::VKSwapChain(const Reference<VKDevice> &device, const vk::SurfaceKHR &surface): mDevice(device), mSurface(surface) {
    QueueFamilyIndex = mDevice->GetPhysicalDevice()->GetQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
}

VKSwapChain::~VKSwapChain() {
    Destroy();
}

void VKSwapChain::Load() {
}

void VKSwapChain::Create(uint32_t width, uint32_t height, bool vsync) {
    // Surface Support
    bool supported = mDevice->GetPhysicalDevice()->Call().getSurfaceSupportKHR(QueueFamilyIndex, mSurface);
    
    // Check SwapChain support details (capabilities, formats, present modes)
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = mDevice->GetPhysicalDevice()->Call().getSurfaceCapabilitiesKHR(mSurface);
    ChooseCapabilities(surfaceCapabilities, width, height);
    vector<vk::SurfaceFormatKHR> surfaceFormats = mDevice->GetPhysicalDevice()->Call().getSurfaceFormatsKHR(mSurface);
    ChooseSurfaceFormat(surfaceFormats);
    vector<vk::PresentModeKHR> surfacePresentModes = mDevice->GetPhysicalDevice()->Call().getSurfacePresentModesKHR(mSurface);
    ChoosePresentModes(surfacePresentModes, vsync);

    // Swapchain Properties
    mDevice->Call().waitIdle();
    vk::SwapchainKHR oldSwapchain = mSwapchain;
    mImageCount = std::clamp(surfaceCapabilities.maxImageCount, 1U, 3U);

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
    createInfo.imageFormat = SurfaceColorFormat;
    createInfo.imageColorSpace = SurfaceColorSpace;
    createInfo.imageExtent = mSurfaceSize;
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
        //AppLogCritical(exception);
    }

    // Destroy previous SwapChain and ImageViews
    if (oldSwapchain != vk::SwapchainKHR(nullptr)) {
        for (uint32_t i = 0; i < mImageCount; i++) {
            mDevice->Call().destroyImageView(mSwapchainBuffers[i].View);
        }
        mDevice->Call().destroySwapchainKHR(oldSwapchain);
    }
    
    CreateImageViews();
    CreateDepthStencilBuffer();
    CreateRenderPass();
    CreatePipeline();
    CreateFrameBuffer();
    CreateDrawBuffers();
    CreateSynchronization();
}

void VKSwapChain::CreateImageViews() {
    mDevice->Call().getSwapchainImagesKHR(mSwapchain, &mImageCount, nullptr);
    mImages.resize(mImageCount);
    mDevice->Call().getSwapchainImagesKHR(mSwapchain, &mImageCount, mImages.data());

    mSwapchainBuffers.resize(mImageCount);
    for (size_t i = 0; i < mImageCount; i++) {
        vk::ImageViewCreateInfo createInfo = {};
        createInfo.format = SurfaceColorFormat;
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.components = {  // eIdentity for all?
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

        mSwapchainBuffers[i].Image = mImages[i];
        createInfo.image = mSwapchainBuffers[i].Image;

        mSwapchainBuffers[i].View = mDevice->Call().createImageView(createInfo);
    }
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

void VKSwapChain::CreatePipeline() {
    auto vertexShaderCode = ReadFile("assets/shaders/Basic.vert.spv");
    auto vertexShaderModule = mDevice->Call().createShaderModuleUnique(
        vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlags(),
            vertexShaderCode.size(),
            reinterpret_cast<const uint32_t *>(vertexShaderCode.data())
        )
    );

    auto fragmentShaderCode = ReadFile("assets/shaders/Basic.frag.spv");
    auto fragmentShaderModule = mDevice->Call().createShaderModuleUnique(
        vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlags(),
            fragmentShaderCode.size(),
            reinterpret_cast<const uint32_t *>(fragmentShaderCode.data())
        )
    );

    vk::PipelineShaderStageCreateInfo shaderStages[] = { 
        {
            vk::PipelineShaderStageCreateFlags(),
            vk::ShaderStageFlagBits::eVertex,
            *vertexShaderModule,
            "main"
        }, 
        {
            vk::PipelineShaderStageCreateFlags(),
            vk::ShaderStageFlagBits::eFragment,
            *fragmentShaderModule,
            "main"
        } 
    };

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    vk::PipelineViewportStateCreateInfo viewportState = {};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &mViewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &mRenderArea;

    vk::PipelineDepthStencilStateCreateInfo depthStencilState = {};
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;
    depthStencilState.back.failOp = vk::StencilOp::eKeep;
    depthStencilState.back.passOp = vk::StencilOp::eKeep;
    depthStencilState.back.compareOp = vk::CompareOp::eAlways;
    depthStencilState.stencilTestEnable = VK_FALSE;
    depthStencilState.front = depthStencilState.back;

    vk::PipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = VK_FALSE;

    vk::PipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_FALSE;

    vk::PipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 0;

    mPipelineLayout = mDevice->Call().createPipelineLayout(pipelineLayoutInfo);


    vk::GraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.layout = mPipelineLayout;
    pipelineInfo.renderPass = RenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;

    auto result = mDevice->Call().createGraphicsPipeline(nullptr, pipelineInfo);
    mPipeline = result.value;
}

void VKSwapChain::CreateFrameBuffer() {
    vector<vk::ImageView> attachments;
    attachments.resize(2);
    attachments[1] = mDepthStencil.View;

    vk::FramebufferCreateInfo createInfo = {};
    createInfo.renderPass = RenderPass;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.width = mSurfaceSize.width;
    createInfo.height = mSurfaceSize.height;
    createInfo.layers = 1;

    mFramebuffers.resize(mImageCount);
    for (size_t i = 0; i < mFramebuffers.size(); i++) {
        attachments[0] = mSwapchainBuffers[i].View;
        mFramebuffers[i] = mDevice->Call().createFramebuffer(createInfo);
    }
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

    for (size_t i = 0; i < mDrawCommandBuffers.size(); i++) {
        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
        mDrawCommandBuffers[i].begin(beginInfo);

        vk::RenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.renderPass = RenderPass;
        renderPassInfo.framebuffer = mFramebuffers[i];
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = mSurfaceSize;

        vk::ClearValue clearValues[2];
        clearValues[0].color = array<float, 4> { 0.1f, 0.1f,0.1f, 1.0f};
        clearValues[1].depthStencil = { 1.0f, 0 };

        renderPassInfo.clearValueCount = 2;
        renderPassInfo.pClearValues = clearValues;

        mDrawCommandBuffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
        mDrawCommandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline);
        mDrawCommandBuffers[i].draw(3, 1, 0, 0);
        mDrawCommandBuffers[i].endRenderPass();

        mDrawCommandBuffers[i].end();
    }
}

void VKSwapChain::CreateSynchronization() {
    mSemaphores.PresentComplete.resize(mSwapchainBuffers.size());
    mSemaphores.RenderComplete.resize(mSwapchainBuffers.size());
    mWaitFences.resize(mSwapchainBuffers.size());

    for (size_t i = 0; i < mSwapchainBuffers.size(); i++) {
        // Semaphore used to ensures that image presentation is complete before starting to submit again
        mSemaphores.PresentComplete[i] = mDevice->Call().createSemaphore(vk::SemaphoreCreateInfo());

        // Semaphore used to ensures that all commands submitted have been finished before submitting the image to the queue
        mSemaphores.RenderComplete[i] = mDevice->Call().createSemaphore(vk::SemaphoreCreateInfo());

        // Fence for command buffer completion
        mWaitFences[i] = mDevice->Call().createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }
}


void VKSwapChain::ChooseCapabilities(const vk::SurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height) {
    if (!(capabilities.currentExtent.width == -1 || capabilities.currentExtent.height == -1)) {
        mSurfaceSize = capabilities.currentExtent;
    } else {
        mSurfaceSize = vk::Extent2D(
            std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
            std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
        );
    }
    mRenderArea = vk::Rect2D(vk::Offset2D(), mSurfaceSize);
    mViewport = vk::Viewport(0.0f, 0.0f, static_cast<float>(mSurfaceSize.width), static_cast<float>(mSurfaceSize.height), 0, 1.0f);
}

void VKSwapChain::ChooseSurfaceFormat(const vector<vk::SurfaceFormatKHR> &surfaceFormats) {
    // Color Buffer
    if (surfaceFormats.size() == 1 && surfaceFormats[0].format == vk::Format::eUndefined) {
        SurfaceColorFormat = vk::Format::eB8G8R8A8Unorm;
        SurfaceColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    } else {
        SurfaceColorFormat = surfaceFormats[0].format;
        SurfaceColorSpace = surfaceFormats[0].colorSpace;
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
            SurfaceDepthFormat = format;
            break;
        }
    }
}

void VKSwapChain::ChoosePresentModes(const vector<vk::PresentModeKHR> &presentModes, bool sync) {
    mPresentMode = vk::PresentModeKHR::eFifo;

    if (!sync) {
        for (auto &mode : presentModes) {
            switch (mode) {
                case vk::PresentModeKHR::eMailbox:      { mPresentMode = vk::PresentModeKHR::eMailbox;   break; }
                case vk::PresentModeKHR::eImmediate:    { mPresentMode = vk::PresentModeKHR::eImmediate; break; }
                default:                                { break;}
            }
        }
    }
}

void VKSwapChain::Destroy() {
    mDevice->Call().waitIdle();

    for (vk::Semaphore &semaphore : mSemaphores.PresentComplete) {
        mDevice->Call().destroySemaphore(semaphore);
    }
    for (vk::Semaphore &semaphore : mSemaphores.RenderComplete) {
        mDevice->Call().destroySemaphore(semaphore);
    }
    for (vk::Fence &fence : mWaitFences) {
        mDevice->Call().destroyFence(fence);
    }

    mDevice->Call().freeCommandBuffers(mDevice->GetCommandPool(), mDrawCommandBuffers);
    mDevice->Call().destroyCommandPool(mCommandPool);

    mDevice->Call().destroyPipeline(mPipeline);
    mDevice->Call().destroyPipelineLayout(mPipelineLayout);
    mDevice->Call().destroyRenderPass(RenderPass);

    for (auto buffer : mSwapchainBuffers) {
        mDevice->Call().destroyImageView(buffer.View);
    }

    mDevice->Call().destroySwapchainKHR(mSwapchain);

}

void VKSwapChain::Cleanup() {
    Destroy();
    if (mSwapchain) {
        for (size_t i = 0; i < mImageCount; i++) {
            mDevice->Call().destroyImageView(mSwapchainBuffers[i].View);
        }
        mDevice->Call().destroySwapchainKHR(mSwapchain);
        mSwapchain = nullptr;
    }
}

void VKSwapChain::Resize(uint32_t width, uint32_t height) {
    return;
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
    AquireNextImage(mSemaphores.PresentComplete[CurrentBufferIndex], &CurrentBufferIndex);
}

void VKSwapChain::Test() {
    mDevice->Call().waitForFences(1, &mWaitFences[CurrentFrame], VK_TRUE, 100);
    mDevice->Call().resetFences(mWaitFences[CurrentFrame]);

    CurrentBufferIndex = mDevice->Call().acquireNextImageKHR(mSwapchain, UINT64_MAX, mSemaphores.PresentComplete[CurrentFrame], nullptr).value;

    vk::SubmitInfo submitInfo = {};
    vk::Semaphore waitSemaphores[] = { mSemaphores.PresentComplete[CurrentFrame] };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mDrawCommandBuffers[CurrentBufferIndex];

    vk::Semaphore signalSemaphores[] = { mSemaphores.RenderComplete[CurrentBufferIndex] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    mDevice->GetQueue().submit(submitInfo, mWaitFences[CurrentFrame]);

    mDevice->Call().waitForFences(1, &mWaitFences[CurrentFrame], VK_TRUE, UINT64_MAX);
    mDevice->Call().resetFences(mWaitFences[CurrentFrame]);


    vk::PresentInfoKHR presentInfo = {};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    vk::SwapchainKHR swapChains[] = { mSwapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &CurrentBufferIndex;
    presentInfo.pResults = nullptr; // Optional

    mDevice->GetQueue().presentKHR(presentInfo);

    CurrentFrame = (CurrentFrame + 1) % mImageCount;
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
    vk::Result result = QueuePresent(mDevice->GetQueue(), CurrentBufferIndex, mSemaphores.RenderComplete[0]);
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
