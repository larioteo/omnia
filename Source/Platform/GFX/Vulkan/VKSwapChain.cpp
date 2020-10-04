#include "VKSwapChain.h"

#include "Omnia/Log.h"
#include "Omnia/System/FileSystem.h"

namespace Omnia {

static vk::CommandBuffer gDrawCommandBuffer;
static vk::CommandBuffer gUICommandBuffer;

// Default
VKSwapChain::VKSwapChain(const Reference<VKDevice> &device, const vk::SurfaceKHR &surface): mDevice(device), mSurface(surface) {
    // Verify if the surface is supported by the selected physical device
    if (!mDevice->GetPhysicalDevice()->Call().getSurfaceSupportKHR(QueueFamilyIndex, mSurface)) {
        AppLogCritical("[GFX::Context::SwapChain] ", "The requested surface isn't supported on the selected physical device!");
    }
    
    QueueFamilyIndex = mDevice->GetPhysicalDevice()->GetQueueFamilyIndex(vk::QueueFlagBits::eGraphics);

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
    mImageCount = std::clamp(surfaceCapabilities.maxImageCount, 1U, 3U);
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
        //AppLogCritical(exception);
    }

    // Destroy previous SwapChain and ImageViews
    if (oldSwapchain != vk::SwapchainKHR(nullptr)) {
        for (uint32_t i = 0; i < mImageCount; i++) {
            //mDevice->Call().destroyImageView(mSwapchainBuffers[i].View);
        }
        mDevice->Call().destroySwapchainKHR(oldSwapchain);
    }
    
    CreateImageViews();
    CreateDepthStencilImageViews();
    CreateRenderPass();
    CreatePipeline();
    CreateFrameBuffer();
    CreateDrawCommandBuffers();

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

    mDevice->Call().freeCommandBuffers(mDevice->GetCommandPool(), mDrawCommandBuffers);

    mDevice->Call().destroyPipeline(mPipeline);
    mDevice->Call().destroyPipelineLayout(mPipelineLayout);
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

    mDevice->Call().freeCommandBuffers(mDevice->GetCommandPool(), static_cast<uint32_t>(mDrawCommandBuffers.size()), mDrawCommandBuffers.data());
    //CreateDrawCommandBuffers();

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
}

// Helpers
void VKSwapChain::CreateImageViews() {
    // Color Image
    vk::ImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.imageType = vk::ImageType::e2D;
    imageCreateInfo.format = mSurfaceProperties.ColorFormat;
    imageCreateInfo.extent = vk::Extent3D(mSurfaceProperties.Size, 1U);
    imageCreateInfo.mipLevels = 1U;
    imageCreateInfo.arrayLayers = 1U;
    imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
    imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
    imageCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled;
    imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imageCreateInfo.queueFamilyIndexCount = 1;
    imageCreateInfo.pQueueFamilyIndices = &QueueFamilyIndex;
    imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    mColorAttachment.Image = mDevice->Call().createImage(imageCreateInfo);

    // Search through GPU memory properies to see if this can be device local
    vk::MemoryRequirements memoryRequirements = mDevice->Call().getImageMemoryRequirements(mColorAttachment.Image);
    mAllocator.Allocate(memoryRequirements, &mColorAttachment.Memory, vk::MemoryPropertyFlagBits::eDeviceLocal);
    mDevice->Call().bindImageMemory(mColorAttachment.Image, mColorAttachment.Memory, 0);

    vector<vk::Image> images;
    mDevice->Call().getSwapchainImagesKHR(mSwapchain, &mImageCount, nullptr);
    images.resize(mImageCount);
    mDevice->Call().getSwapchainImagesKHR(mSwapchain, &mImageCount, images.data());

    mSwapchainBuffers.resize(mImageCount);
    for (size_t i = 0; i < mImageCount; i++) {
        vk::ImageViewCreateInfo createInfo = {};
        createInfo.format = mSurfaceProperties.ColorFormat;
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

        mSwapchainBuffers[i].Image = images[i]; // mColorAttachment.Image
        createInfo.image = mSwapchainBuffers[i].Image;

        mSwapchainBuffers[i].View = mDevice->Call().createImageView(createInfo);
    }

    vk::SamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.maxAnisotropy = 1.0f;
    samplerCreateInfo.magFilter = vk::Filter::eLinear;
    samplerCreateInfo.minFilter = vk::Filter::eLinear;
    samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.maxAnisotropy = 1.0f;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = 1.0f;
    samplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
    mColorAttachmentSampler = mDevice->Call().createSampler(samplerCreateInfo);

    mDescriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    mDescriptorImageInfo.imageView = mColorAttachment.View;
    mDescriptorImageInfo.sampler = mColorAttachmentSampler;
}

void VKSwapChain::CreateDepthStencilImageViews() {
    // Depth\Stencil Image
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
    imageCreateInfo.pQueueFamilyIndices = &QueueFamilyIndex;
    imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    mDepthStencil.Image = mDevice->Call().createImage(imageCreateInfo);

    // Search through GPU memory properies to see if this can be device local
    vk::MemoryRequirements memoryRequirements = mDevice->Call().getImageMemoryRequirements(mDepthStencil.Image);
    mAllocator.Allocate(memoryRequirements, &mDepthStencil.Memory, vk::MemoryPropertyFlagBits::eDeviceLocal);
    mDevice->Call().bindImageMemory(mDepthStencil.Image, mDepthStencil.Memory, 0);

    // Depth\Stencil ImageView
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
            VK_SUBPASS_EXTERNAL,
            0,
            vk::PipelineStageFlagBits::eFragmentShader, // eBottomOfPipe
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::AccessFlagBits::eMemoryRead,
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::DependencyFlagBits::eByRegion
        ),
        vk::SubpassDependency(
            0,
            VK_SUBPASS_EXTERNAL,
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eFragmentShader, // eBottomOfPipe
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
            vk::AccessFlagBits::eMemoryRead,
            vk::DependencyFlagBits::eByRegion
        )
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
    viewportState.pViewports = &mSurfaceProperties.Viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &mSurfaceProperties.RenderArea;

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
    pipelineInfo.renderPass = mRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;

    auto result = mDevice->Call().createGraphicsPipeline(nullptr, pipelineInfo);
    mPipeline = result.value;
}

void VKSwapChain::CreateFrameBuffer() {
    vector<vk::ImageView> attachments;
    attachments.resize(2);
    //attachments[1] = mColorAttachment.View;
    attachments[1] = mDepthStencil.View;

    vk::FramebufferCreateInfo createInfo = {};
    createInfo.renderPass = mRenderPass;
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();
    createInfo.width = mSurfaceProperties.Size.width;
    createInfo.height = mSurfaceProperties.Size.height;
    createInfo.layers = 1;

    for (size_t i = 0; i < mSwapchainBuffers.size(); i++) {
        attachments[0] = mSwapchainBuffers[i].View;
        mSwapchainBuffers[i] .FrameBuffer= mDevice->Call().createFramebuffer(createInfo);
    }
}

void VKSwapChain::CreateDrawCommandBuffers() {
    mDrawCommandBuffers.resize(mImageCount);

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo = {};
    commandBufferAllocateInfo.commandPool = mDevice->GetCommandPool();
    commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
    commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(mDrawCommandBuffers.size());
    mDrawCommandBuffers = mDevice->Call().allocateCommandBuffers(commandBufferAllocateInfo);

    for (size_t i = 0; i < mDrawCommandBuffers.size(); i++) {
        vk::CommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
        mDrawCommandBuffers[i].begin(beginInfo);

        vk::RenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.renderPass = mRenderPass;
        renderPassInfo.framebuffer = mSwapchainBuffers[i].FrameBuffer;
        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = mSurfaceProperties.Size;
        renderPassInfo.clearValueCount = 2;
        renderPassInfo.pClearValues = mSurfaceProperties.ClearValues;

        mDrawCommandBuffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
        mDrawCommandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, mPipeline);
        mDrawCommandBuffers[i].draw(3, 1, 0, 0);
        mDrawCommandBuffers[i].endRenderPass();

        mDrawCommandBuffers[i].end();
    }
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

const vk::Pipeline & VKSwapChain::GetPipeline() const {
    return mPipeline;
}

const vk::Rect2D & VKSwapChain::GetRenderArea() const {
    return mSurfaceProperties.RenderArea;
}

const vk::RenderPass &VKSwapChain::GetRenderPass() const {
    return mRenderPass;
}

vk::CommandBuffer VKSwapChain::PrepareRenderPass() {
    gDrawCommandBuffer = GetCurrentDrawCommandBuffer();

    vk::RenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.renderPass = mRenderPass;
    renderPassInfo.renderArea.offset.x = 0;
    renderPassInfo.renderArea.offset.y = 0;
    renderPassInfo.framebuffer = GetCurrentFramebuffer();
    renderPassInfo.renderArea.extent = mSurfaceProperties.Size;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = mSurfaceProperties.ClearValues;

    gDrawCommandBuffer.begin(vk::CommandBufferBeginInfo());

    // Primary CommandBuffer
    {
        gDrawCommandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline); 

        gDrawCommandBuffer.setViewport(0, mSurfaceProperties.Viewport);
        gDrawCommandBuffer.setScissor(0, mSurfaceProperties.RenderArea);

        // ToDo: Here can something placed for rendering later

        gDrawCommandBuffer.endRenderPass();
    }

    gUICommandBuffer = mDevice->CreateSecondaryCommandBuffer();
    // Secondary CommandBuffers
    {
        gDrawCommandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eSecondaryCommandBuffers);

        vk::CommandBufferInheritanceInfo inheritanceInfo = {};
        inheritanceInfo.renderPass = mRenderPass;
        inheritanceInfo.framebuffer = GetCurrentFramebuffer();

        vk::CommandBufferBeginInfo bufferInfo = {};
        bufferInfo.flags = vk::CommandBufferUsageFlagBits::eRenderPassContinue;
        bufferInfo.pInheritanceInfo = &inheritanceInfo;

        gUICommandBuffer.begin(bufferInfo);

        gUICommandBuffer.setViewport(0, mSurfaceProperties.Viewport);
        gUICommandBuffer.setScissor(0, mSurfaceProperties.RenderArea);

        // The Commands here are currently external
    }

    return gUICommandBuffer;
}

void VKSwapChain::FinishRenderPass() {
    gUICommandBuffer.end();

    gDrawCommandBuffer.executeCommands(1, &gUICommandBuffer);
    gDrawCommandBuffer.endRenderPass();
    gDrawCommandBuffer.end();
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
