#include "VKContext.h"

#include "Omnia/Log.h"
#include "Omnia/UI/Window.h"

#include "Omnia/System/FileSystem.h"
#include "../../3rd-Party/.Library/glm/glm.hpp"
#include "../../3rd-Party/.Library/glm/gtc/matrix_transform.hpp"

namespace Omnia {

uint32_t GetMemoryTypeIndex2(const vk::PhysicalDevice &physicalDevice, uint32_t typeBits, vk::MemoryPropertyFlags properties) {
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

VKContext::VKContext(void *window) {
    WindowHandle = reinterpret_cast<HWND>(window);
    mInstance = CreateReference<VKInstance>();
    mPhysicalDevice = CreateReference<VKPhysicalDevice>(mInstance->Call());
    mDevice = CreateReference<VKDevice>(mPhysicalDevice);
}

VKContext::~VKContext() {
    mDevice->GetDevice().waitIdle();
    DestroyCommands();
    DestroyResources();
    
    // Destroy API
    mDevice->Destroy();
    mInstance->Call().destroySurfaceKHR(Surface);
    mInstance->Call().destroy();
}

void VKContext::Load() {
    // Sources: https://alain.xyz/blog/raw-vulkan | https://gist.github.com/graphitemaster/e162a24e57379af840d4
    vk::Result result;
    QueueFamilyIndex = mPhysicalDevice->mQueueFamilyIndices.Graphics;

    // Surface
    VkSurfaceKHR surface;
    VkResult resultN = VK_RESULT_MAX_ENUM;
    VkWin32SurfaceCreateInfoKHR surfaceCreateinfo;
    surfaceCreateinfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateinfo.pNext = NULL;
    surfaceCreateinfo.flags = 0;
    surfaceCreateinfo.hinstance = GetModuleHandle(NULL);
    surfaceCreateinfo.hwnd = WindowHandle;
    Surface = mInstance->Call().createWin32SurfaceKHR(surfaceCreateinfo);


    // Swapchain
    mSwapChain = CreateReference<VKSwapChain>(mInstance->Call(), mDevice, Surface);
    mSwapChain->Create(1280, 1024);

    // Command Buffers
    CreateCommands();

    // Sync
    CreateSynchronization();

    LoadResources();
    SetupCommands();
}

void VKContext::Attach() {}
void VKContext::Detach() {}

void *VKContext::GetNativeContext() {
    static VkContextData *data = new VkContextData();
    data->Intance = mInstance->Call();
    data->PhysicalDevice = mPhysicalDevice->GePhysicalDevice();
    data->Device = mDevice->GetDevice();
    data->QueueIndex = mPhysicalDevice->mQueueFamilyIndices.Graphics;
    data->Queue = mDevice->GetQueue();
    data->PipelineCache = PipelineCache;
    data->DescriptorPool = DescriptorPool;
    data->RenderPass = mSwapChain->RenderPass;
    data->Surface = Surface;
    data->Swapchain = mSwapChain->mSwapchain;
    data->CommandPool = mDevice->mCommandPool;
    data->Semaphore = RenderCompleteSemaphore;
    return (void*)data;
}

bool const VKContext::IsCurrentContext() {
    return nullptr;
}

void VKContext::SetViewport(uint32_t width, uint32_t height, int32_t x, int32_t y) {
    mDevice->GetDevice().waitIdle();
    mSwapChain->DestroyFrameBuffer();
    mSwapChain->Create(width, height);
    mSwapChain->LoadFrameBuffer();
    DestroyCommands();
    CreateCommands();
    SetupCommands();
    mDevice->GetDevice().waitIdle();

    // Uniforms
    //uboVS.projectionMatrix = glm::perspective(45.0f, (float)Viewport.width / (float)Viewport.height, 0.01f, 1024.0f);
}

void VKContext::SwapBuffers() {
    RenderTest(Timestamp());
}

void VKContext::SetVSync(bool activate) {}






// ToDo: CleanUp everything not needed...
struct Vertex {
    float position[3];
    float color[3];
};

Vertex VertexBufferData[3] = {
    { { 1.0f,  1.0f, 0.0f },{ 1.0f, 0.0f, 0.0f } },
    { { -1.0f,  1.0f, 0.0f },{ 0.0f, 1.0f, 0.0f } },
    { { 0.0f, -1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } }
};

uint32_t IndexBufferData[3] = { 0, 1, 2 };

// Uniform data
struct {
    glm::mat4 projectionMatrix;
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
} uboVS;

void VKContext::LoadResources() {
    /**
    * Create Shader uniform binding data structures:
    */

    //Descriptor Pool
    vector<vk::DescriptorPoolSize> descriptorPoolSizes = { vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1) };

    DescriptorPool = mDevice->GetDevice().createDescriptorPool(
        vk::DescriptorPoolCreateInfo( vk::DescriptorPoolCreateFlags(), 1, static_cast<uint32_t>(descriptorPoolSizes.size()),  descriptorPoolSizes.data())
    );

    // Descriptor Set Layout ~ Binding 0: Uniform buffer (Vertex shader)
    vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr)
    };

    DescriptorSetLayouts = {
        mDevice->GetDevice().createDescriptorSetLayout( vk::DescriptorSetLayoutCreateInfo( vk::DescriptorSetLayoutCreateFlags(), static_cast<uint32_t>(descriptorSetLayoutBindings.size()), descriptorSetLayoutBindings.data()))
    };

    DescriptorSets = mDevice->GetDevice().allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo(DescriptorPool, static_cast<uint32_t>(DescriptorSetLayouts.size()), DescriptorSetLayouts.data())
    );

    PipelineLayout = mDevice->GetDevice().createPipelineLayout(
        vk::PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), static_cast<uint32_t>(DescriptorSetLayouts.size()), DescriptorSetLayouts.data(), 0, nullptr)
    );

    // Setup vertices data
    uint32_t vertexBufferSize = static_cast<uint32_t>(3) * sizeof(Vertex);

    // Setup mIndices data
    Indices.count = 3;
    uint32_t indexBufferSize = Indices.count * sizeof(uint32_t);

    void *data;
    // Static data like vertex and index buffer should be stored on the device memory  for optimal (and fastest) access by the GPU
    //
    // To achieve this we use so-called "staging buffers" :
    // - Create a buffer that's visible to the host (and can be mapped)
    // - Copy the data to this buffer
    // - Create another buffer that's local on the device (VRAM) with the same size
    // - Copy the data from the host to the device using a command buffer
    // - Delete the host visible (staging) buffer
    // - Use the device local buffers for rendering

    struct StagingBuffer {
        vk::DeviceMemory memory;
        vk::Buffer buffer;
    };

    struct {
        StagingBuffer vertices;
        StagingBuffer indices;
    } stagingBuffers;

    // Vertex buffer
    stagingBuffers.vertices.buffer = mDevice->GetDevice().createBuffer(
        vk::BufferCreateInfo(
        vk::BufferCreateFlags(),
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::SharingMode::eExclusive,
        1,
        &QueueFamilyIndex
    )
    );

    auto memReqs = mDevice->GetDevice().getBufferMemoryRequirements(stagingBuffers.vertices.buffer);

    // Request a host visible memory type that can be used to copy our data do
    // Also request it to be coherent, so that writes are visible to the GPU right after unmapping the buffer
    stagingBuffers.vertices.memory = mDevice->GetDevice().allocateMemory(
        vk::MemoryAllocateInfo(
        memReqs.size,
        GetMemoryTypeIndex2(mPhysicalDevice->GePhysicalDevice(), memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
    )
    );

    // Map and copy
    data = mDevice->GetDevice().mapMemory(stagingBuffers.vertices.memory, 0, memReqs.size, vk::MemoryMapFlags());
    memcpy(data, VertexBufferData, vertexBufferSize);
    mDevice->GetDevice().unmapMemory(stagingBuffers.vertices.memory);
    mDevice->GetDevice().bindBufferMemory(stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0);

    // Create a device local buffer to which the (host local) vertex data will be copied and which will be used for rendering
    Vertices.buffer = mDevice->GetDevice().createBuffer(
        vk::BufferCreateInfo(
        vk::BufferCreateFlags(),
        vertexBufferSize,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::SharingMode::eExclusive,
        1,
        &QueueFamilyIndex
    )
    );

    memReqs = mDevice->GetDevice().getBufferMemoryRequirements(Vertices.buffer);

    Vertices.memory = mDevice->GetDevice().allocateMemory(
        vk::MemoryAllocateInfo(
        memReqs.size,
        GetMemoryTypeIndex2(mPhysicalDevice->GePhysicalDevice(), memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
    )
    );

    mDevice->GetDevice().bindBufferMemory(Vertices.buffer, Vertices.memory, 0);

    // Index buffer
    // Copy index data to a buffer visible to the host (staging buffer)
    stagingBuffers.indices.buffer = mDevice->GetDevice().createBuffer(
        vk::BufferCreateInfo(
        vk::BufferCreateFlags(),
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::SharingMode::eExclusive,
        1,
        &QueueFamilyIndex
    )
    );
    memReqs = mDevice->GetDevice().getBufferMemoryRequirements(stagingBuffers.indices.buffer);
    stagingBuffers.indices.memory = mDevice->GetDevice().allocateMemory(
        vk::MemoryAllocateInfo(
        memReqs.size,
        GetMemoryTypeIndex2(mPhysicalDevice->GePhysicalDevice(), memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
    )
    );

    data = mDevice->GetDevice().mapMemory(stagingBuffers.indices.memory, 0, indexBufferSize, vk::MemoryMapFlags());
    memcpy(data, IndexBufferData, indexBufferSize);
    mDevice->GetDevice().unmapMemory(stagingBuffers.indices.memory);
    mDevice->GetDevice().bindBufferMemory(stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0);

    // Create destination buffer with device only visibility
    Indices.buffer = mDevice->GetDevice().createBuffer(
        vk::BufferCreateInfo(
        vk::BufferCreateFlags(),
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::SharingMode::eExclusive,
        0,
        nullptr
    )
    );

    memReqs = mDevice->GetDevice().getBufferMemoryRequirements(Indices.buffer);
    Indices.memory = mDevice->GetDevice().allocateMemory(
        vk::MemoryAllocateInfo(
        memReqs.size,
        GetMemoryTypeIndex2(mPhysicalDevice->GePhysicalDevice(), memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal
    )
    )
    );

    mDevice->GetDevice().bindBufferMemory(Indices.buffer, Indices.memory, 0);

    auto getCommandBuffer = [&](bool begin) {
        vk::CommandBuffer cmdBuffer = mDevice->GetDevice().allocateCommandBuffers (
            vk::CommandBufferAllocateInfo(
            mDevice->mCommandPool,
            vk::CommandBufferLevel::ePrimary,
            1)
        )[0];

        // If requested, also start the new command buffer
        if (begin) {
            cmdBuffer.begin(
                vk::CommandBufferBeginInfo()
            );
        }

        return cmdBuffer;
    };

    // Buffer copies have to be submitted to a queue, so we need a command buffer for them
    // Note: Some devices offer a dedicated transfer queue (with only the transfer bit set) that may be faster when doing lots of copies
    vk::CommandBuffer copyCmd = getCommandBuffer(true);

    // Put buffer region copies into command buffer
    vector<vk::BufferCopy> copyRegions = {
        vk::BufferCopy(0, 0, vertexBufferSize)
    };

    // Vertex buffer
    copyCmd.copyBuffer(stagingBuffers.vertices.buffer, Vertices.buffer, copyRegions);

    // Index buffer
    copyRegions =
    {
        vk::BufferCopy(0, 0,  indexBufferSize)
    };

    copyCmd.copyBuffer(stagingBuffers.indices.buffer, Indices.buffer, copyRegions);

    // Flushing the command buffer will also submit it to the queue and uses a fence to ensure that all commands have been executed before returning
    auto flushCommandBuffer = [&](vk::CommandBuffer commandBuffer)
    {
        commandBuffer.end();

        vector<vk::SubmitInfo> submitInfos = {
            vk::SubmitInfo(0, nullptr, nullptr, 1, &commandBuffer, 0, nullptr)
        };

        // Create fence to ensure that the command buffer has finished executing
        vk::Fence fence = mDevice->GetDevice().createFence(vk::FenceCreateInfo());

        // Submit to the queue
        mDevice->GetQueue().submit(submitInfos, fence);
        // Wait for the fence to signal that command buffer has finished executing
        mDevice->GetDevice().waitForFences(1, &fence, VK_TRUE, UINT_MAX);
        mDevice->GetDevice().destroyFence(fence);
        mDevice->GetDevice().freeCommandBuffers(mDevice->mCommandPool, 1, &commandBuffer);
    };

    flushCommandBuffer(copyCmd);

    // Destroy staging buffers
    // Note: Staging buffer must not be deleted before the copies have been submitted and executed
    mDevice->GetDevice().destroyBuffer(stagingBuffers.vertices.buffer);
    mDevice->GetDevice().freeMemory(stagingBuffers.vertices.memory);
    mDevice->GetDevice().destroyBuffer(stagingBuffers.indices.buffer);
    mDevice->GetDevice().freeMemory(stagingBuffers.indices.memory);


    // Vertex input binding
    Vertices.inputBinding.binding = 0;
    Vertices.inputBinding.stride = sizeof(Vertex);
    Vertices.inputBinding.inputRate = vk::VertexInputRate::eVertex;

    // Inpute attribute binding describe shader attribute locations and memory layouts
    // These match the following shader layout (see assets/shaders/triangle.vert):
    //	layout (location = 0) in vec3 inPos;
    //	layout (location = 1) in vec3 inColor;
    Vertices.inputAttributes.resize(2);
    // Attribute location 0: Position
    Vertices.inputAttributes[0].binding = 0;
    Vertices.inputAttributes[0].location = 0;
    Vertices.inputAttributes[0].format = vk::Format::eR32G32B32Sfloat;
    Vertices.inputAttributes[0].offset = offsetof(Vertex, position);
    // Attribute location 1: Color
    Vertices.inputAttributes[1].binding = 0;
    Vertices.inputAttributes[1].location = 1;
    Vertices.inputAttributes[1].format = vk::Format::eR32G32B32Sfloat;
    Vertices.inputAttributes[1].offset = offsetof(Vertex, color);

    // Assign to the vertex input state used for pipeline creation
    Vertices.inputState.flags = vk::PipelineVertexInputStateCreateFlags();
    Vertices.inputState.vertexBindingDescriptionCount = 1;
    Vertices.inputState.pVertexBindingDescriptions = &Vertices.inputBinding;
    Vertices.inputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(Vertices.inputAttributes.size());
    Vertices.inputState.pVertexAttributeDescriptions = Vertices.inputAttributes.data();

    // Prepare and initialize a uniform buffer block containing shader uniforms
    // Single uniforms like in OpenGL are no longer present in Vulkan. All Shader uniforms are passed via uniform buffer blocks

    // Vertex shader uniform buffer block
    vk::MemoryAllocateInfo allocInfo = {};
    allocInfo.pNext = nullptr;
    allocInfo.allocationSize = 0;
    allocInfo.memoryTypeIndex = 0;

    // Create a new buffer
    UniformDataVS.buffer = mDevice->GetDevice().createBuffer(
        vk::BufferCreateInfo(
        vk::BufferCreateFlags(),
        sizeof(uboVS),
        vk::BufferUsageFlagBits::eUniformBuffer
    )
    );
    // Get memory requirements including size, alignment and memory type 
    memReqs = mDevice->GetDevice().getBufferMemoryRequirements(UniformDataVS.buffer);
    allocInfo.allocationSize = memReqs.size;
    // Get the memory type index that supports host visible memory access
    // Most implementations offer multiple memory types and selecting the correct one to allocate memory from is crucial
    // We also want the buffer to be host coherent so we don't have to flush (or sync after every update.
    // Note: This may affect performance so you might not want to do this in a real world application that updates buffers on a regular base
    allocInfo.memoryTypeIndex = GetMemoryTypeIndex2(mPhysicalDevice->GePhysicalDevice(), memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    // Allocate memory for the uniform buffer
    UniformDataVS.memory = mDevice->GetDevice().allocateMemory(allocInfo);
    // Bind memory to buffer
    mDevice->GetDevice().bindBufferMemory(UniformDataVS.buffer, UniformDataVS.memory, 0);

    // Store information in the uniform's descriptor that is used by the descriptor set
    UniformDataVS.descriptor.buffer = UniformDataVS.buffer;
    UniformDataVS.descriptor.offset = 0;
    UniformDataVS.descriptor.range = sizeof(uboVS);

    // Update Uniforms
    float zoom = -2.5f;

    // Update matrices
    uboVS.projectionMatrix = glm::perspective(45.0f, (float)mSwapChain->mViewport.width / (float)mSwapChain->mViewport.height, 0.01f, 1024.0f);
    uboVS.viewMatrix = glm::translate(glm::identity<glm::mat4>(), { 0.0f, 0.0f, zoom });
    uboVS.modelMatrix = glm::identity<glm::mat4>();

    // Map uniform buffer and update it
    void *pData;
    pData = mDevice->GetDevice().mapMemory(UniformDataVS.memory, 0, sizeof(uboVS));
    memcpy(pData, &uboVS, sizeof(uboVS));
    mDevice->GetDevice().unmapMemory(UniformDataVS.memory);


    vector<vk::WriteDescriptorSet> descriptorWrites = {
        vk::WriteDescriptorSet(
            DescriptorSets[0],
        0,
        0,
        1,
        vk::DescriptorType::eUniformBuffer,
        nullptr,
        &UniformDataVS.descriptor,
        nullptr
        )
    };

    mDevice->GetDevice().updateDescriptorSets(descriptorWrites, nullptr);

    // Create Render Pass
    mSwapChain->CreateRenderPass();
    mSwapChain->LoadFrameBuffer();

    // Create Graphics Pipeline

    string vertShaderCode = ReadFile("Assets/Shaders/triangle.vert.spv");
    string fragShaderCode = ReadFile("Assets/Shaders/triangle.frag.spv");

    VertModule = mDevice->GetDevice().createShaderModule(
        vk::ShaderModuleCreateInfo(
        vk::ShaderModuleCreateFlags(),
        vertShaderCode.size(),
        (uint32_t*)vertShaderCode.data()
    )
    );

    FragModule = mDevice->GetDevice().createShaderModule(
        vk::ShaderModuleCreateInfo(
        vk::ShaderModuleCreateFlags(),
        fragShaderCode.size(),
        (uint32_t*)fragShaderCode.data()
    )
    );

    PipelineCache = mDevice->GetDevice().createPipelineCache(vk::PipelineCacheCreateInfo());

    std::vector<vk::PipelineShaderStageCreateInfo> pipelineShaderStages = {
        vk::PipelineShaderStageCreateInfo(
            vk::PipelineShaderStageCreateFlags(),
        vk::ShaderStageFlagBits::eVertex,
        VertModule,
        "main",
        nullptr
        ),
        vk::PipelineShaderStageCreateInfo(
        vk::PipelineShaderStageCreateFlags(),
        vk::ShaderStageFlagBits::eFragment,
        FragModule,
        "main",
        nullptr
        )
    };

    vk::PipelineVertexInputStateCreateInfo pvi = Vertices.inputState;

    vk::PipelineInputAssemblyStateCreateInfo pia(
        vk::PipelineInputAssemblyStateCreateFlags(),
        vk::PrimitiveTopology::eTriangleList
    );

    vk::PipelineViewportStateCreateInfo pv(
        vk::PipelineViewportStateCreateFlagBits(),
        1,
        &mSwapChain->mViewport,
        1,
        &mSwapChain->mRenderArea
    );

    vk::PipelineRasterizationStateCreateInfo pr(
        vk::PipelineRasterizationStateCreateFlags(),
        VK_FALSE,
        VK_FALSE,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eNone,
        vk::FrontFace::eCounterClockwise,
        VK_FALSE,
        0,
        0,
        0,
        1.0f
    );

    vk::PipelineMultisampleStateCreateInfo pm(
        vk::PipelineMultisampleStateCreateFlags(),
        vk::SampleCountFlagBits::e1
    );

    // Dept and Stencil state for primative compare/test operations
    vk::PipelineDepthStencilStateCreateInfo pds = vk::PipelineDepthStencilStateCreateInfo (
        vk::PipelineDepthStencilStateCreateFlags(),
        VK_TRUE,
        VK_TRUE,
        vk::CompareOp::eLessOrEqual,
        VK_FALSE,
        VK_FALSE,
        vk::StencilOpState(),
        vk::StencilOpState(),
        0,
        0
    );

    // Blend State - How two primatives should draw on top of each other.
    vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments = {
        vk::PipelineColorBlendAttachmentState(
            VK_FALSE,
        vk::BlendFactor::eZero,
        vk::BlendFactor::eOne,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eZero,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
        )
    };

    vk::PipelineColorBlendStateCreateInfo pbs(
        vk::PipelineColorBlendStateCreateFlags(),
        0,
        vk::LogicOp::eClear,
        static_cast<uint32_t>(colorBlendAttachments.size()),
        colorBlendAttachments.data()
    );

    vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo pdy(
        vk::PipelineDynamicStateCreateFlags(),
        static_cast<uint32_t>(dynamicStates.size()),
        dynamicStates.data()
    );

    Pipeline = static_cast<vk::Pipeline>(mDevice->GetDevice().createGraphicsPipeline(
        PipelineCache,
        vk::GraphicsPipelineCreateInfo(
        vk::PipelineCreateFlags(),
        static_cast<uint32_t>(pipelineShaderStages.size()),
        pipelineShaderStages.data(),
        &pvi,
        &pia,
        nullptr,
        &pv,
        &pr,
        &pm,
        &pds,
        &pbs,
        &pdy,
        PipelineLayout,
        mSwapChain->RenderPass,
        0
        )
        ));
}

void VKContext::DestroyResources() {
    // Vertices
    mDevice->GetDevice().freeMemory(Vertices.memory);
    mDevice->GetDevice().destroyBuffer(Vertices.buffer);

    // Index buffer
    mDevice->GetDevice().freeMemory(Indices.memory);
    mDevice->GetDevice().destroyBuffer(Indices.buffer);

    // Shader Module
    mDevice->GetDevice().destroyShaderModule(VertModule);
    mDevice->GetDevice().destroyShaderModule(FragModule);

    // Render Pass
    mDevice->GetDevice().destroyRenderPass(mSwapChain->RenderPass);

    // Graphics Pipeline
    mDevice->GetDevice().destroyPipelineCache(PipelineCache);
    mDevice->GetDevice().destroyPipeline(Pipeline);
    mDevice->GetDevice().destroyPipelineLayout(PipelineLayout);

    // Descriptor Pool
    mDevice->GetDevice().destroyDescriptorPool(DescriptorPool);
    for (vk::DescriptorSetLayout &dsl : DescriptorSetLayouts) {
        mDevice->GetDevice().destroyDescriptorSetLayout(dsl);
    }

    // Uniform block object
    mDevice->GetDevice().freeMemory(UniformDataVS.memory);
    mDevice->GetDevice().destroyBuffer(UniformDataVS.buffer);

    // Destroy Framebuffers, Image Views
    mSwapChain->DestroyFrameBuffer();
    mDevice->GetDevice().destroySwapchainKHR(mSwapChain->mSwapchain);

    // Sync
    mDevice->GetDevice().destroySemaphore(PresentCompleteSemaphore);
    mDevice->GetDevice().destroySemaphore(RenderCompleteSemaphore);
    for (vk::Fence &f : WaitFences) {
        mDevice->GetDevice().destroyFence(f);
    }

}

void VKContext::SetupCommands() {
    vector<vk::ClearValue> clearValues = {
        vk::ClearColorValue(
            std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.0f}),
        vk::ClearDepthStencilValue(1.0f, 0)
    };

    for (size_t i = 0; i < CommandBuffers.size(); ++i){
        vk::CommandBuffer& cmd = CommandBuffers[i];
        cmd.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
        cmd.begin(vk::CommandBufferBeginInfo());
        cmd.beginRenderPass(
            vk::RenderPassBeginInfo(
            mSwapChain->RenderPass,
            mSwapChain->GetSwapchainBuffer()[i].frameBuffer,
            mSwapChain->mRenderArea,
            static_cast<uint32_t>(clearValues.size()),
            clearValues.data()),
            vk::SubpassContents::eInline);

        cmd.setViewport(0, 1, &mSwapChain->mViewport);

        cmd.setScissor(0, 1, &mSwapChain->mRenderArea);

        // Bind Descriptor Sets, these are attribute/uniform "descriptions"
        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, Pipeline);

        cmd.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            PipelineLayout,
            0,
            DescriptorSets,
            nullptr
        );

        vk::DeviceSize offsets = 0;
        cmd.bindVertexBuffers(0, 1, &Vertices.buffer, &offsets);
        cmd.bindIndexBuffer(Indices.buffer, 0, vk::IndexType::eUint32);
        cmd.drawIndexed(Indices.count, 1, 0, 0, 1);
        cmd.endRenderPass();
        cmd.end();
    }
}

void VKContext::CreateCommands() {
    CommandBuffers = mDevice->GetDevice().allocateCommandBuffers(
        vk::CommandBufferAllocateInfo(
        mDevice->mCommandPool,
        vk::CommandBufferLevel::ePrimary,
        static_cast<uint32_t>(mSwapChain->GetSwapchainBuffer().size())
    )
    );
}

void VKContext::DestroyCommands() {
    mDevice->GetDevice().freeCommandBuffers(mDevice->mCommandPool, CommandBuffers);
}

void VKContext::CreateSynchronization() {
    // Semaphore used to ensures that image presentation is complete before starting to submit again
    PresentCompleteSemaphore = mDevice->GetDevice().createSemaphore(vk::SemaphoreCreateInfo());

    // Semaphore used to ensures that all commands submitted have been finished before submitting the image to the queue
    RenderCompleteSemaphore = mDevice->GetDevice().createSemaphore(vk::SemaphoreCreateInfo());

    // Fence for command buffer completion
    WaitFences.resize(mSwapChain->GetSwapchainBuffer().size());

    for (size_t i = 0; i < WaitFences.size(); i++) {
        WaitFences[i] = mDevice->GetDevice().createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }
}

void VKContext::RenderTest(Timestamp delta) {
    // Swap backbuffers
    vk::Result result;

    result = mDevice->GetDevice().acquireNextImageKHR(mSwapChain->mSwapchain, UINT64_MAX, PresentCompleteSemaphore, nullptr, &CurrentBuffer);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        // Swapchain lost, we'll try again next poll
        SetViewport(mSwapChain->mSurfaceSize.width, mSwapChain->mSurfaceSize.height, 0, 0);
        return;
    }
    if (result == vk::Result::eErrorDeviceLost) exit(1); // driver lost, we'll crash in this case:

    // Update Uniforms
    uboVS.modelMatrix = glm::rotate(uboVS.modelMatrix, (float)(0.001f *delta), glm::vec3( 0.0f, 1.0f, 0.0f ));
    
    void *pData;
    pData = mDevice->GetDevice().mapMemory(UniformDataVS.memory, 0, sizeof(uboVS));
    memcpy(pData, &uboVS, sizeof(uboVS));
    mDevice->GetDevice().unmapMemory(UniformDataVS.memory);

    // Wait for Fences
    mDevice->GetDevice().waitForFences(1, &WaitFences[CurrentBuffer], VK_TRUE, UINT64_MAX);
    mDevice->GetDevice().resetFences(1, &WaitFences[CurrentBuffer]);
    vk::SubmitInfo submitInfo;
    vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    submitInfo
        .setWaitSemaphoreCount(1)
        .setPWaitSemaphores(&PresentCompleteSemaphore)
        .setPWaitDstStageMask(&waitDstStageMask)
        .setCommandBufferCount(1)
        .setPCommandBuffers(&CommandBuffers[CurrentBuffer])
        .setSignalSemaphoreCount(1)
        .setPSignalSemaphores(&RenderCompleteSemaphore);
    result = mDevice->GetQueue().submit(1, &submitInfo, WaitFences[CurrentBuffer]);

    if (result == vk::Result::eErrorDeviceLost)
    {
        // driver lost, we'll crash in this case:
        exit(1);
    }

    result = mDevice->GetQueue().presentKHR(vk::PresentInfoKHR(1, &RenderCompleteSemaphore, 1, &mSwapChain->mSwapchain, &CurrentBuffer, nullptr));

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        // Swapchain lost, we'll try again next poll
        SetViewport(mSwapChain->mSurfaceSize.width, mSwapChain->mSurfaceSize.height, 0, 0);
        return;
    }
}

}
