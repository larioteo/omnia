#include "VKContext.h"

#include "Omnia/Log.h"
#include "Omnia/UI/Window.h"

#include "Omnia/System/FileSystem.h"
#include "../../3rd-Party/.Library/glm/glm.hpp"
#include "../../3rd-Party/.Library/glm/gtc/matrix_transform.hpp"

namespace Omnia {

// ToDo: Only for testing...
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

std::chrono::time_point<std::chrono::steady_clock> tStart, tEnd;
float ElapsedTime = 0.0f;

// Uniform data
struct {
    glm::mat4 projectionMatrix;
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
} uboVS;
// ~

vector<const char *> GetExtensions(const vector<vk::ExtensionProperties>& available, const vector<const char*>& needed) {
    vector<const char *> result = {};
    for (const char* const& n : needed) {
        for (vk::ExtensionProperties const &a : available) {
            if (string(a.extensionName.data()).compare(n) == 0) {
                result.emplace_back(n);
                break;
            }
        }
    }
    return result;
}

vector<const char *> GetLayers(const std::vector<vk::LayerProperties>& available, const std::vector<const char*>& needed) {
    vector<const char *> result = {};
    for (const char* const& n : needed) {
        for (vk::LayerProperties const& a : available) {
            if (string(a.layerName.data()).compare(n) == 0) {
                result.emplace_back(n);
                break;
            }
        }
    }
    return result;
}

uint32_t GetQueueIndex(vk::PhysicalDevice& physicalDevice, vk::QueueFlagBits flags) {
    vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();
    for (size_t i = 0; i < queueProps.size(); ++i) {
        if (queueProps[i].queueFlags & flags) {
            return static_cast<uint32_t>(i);
        }
    }
    return 0;
}

uint32_t GetMemoryTypeIndex(vk::PhysicalDevice& physicalDevice, uint32_t typeBits, vk::MemoryPropertyFlags properties) {
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
}

VKContext::~VKContext() {
    Device.waitIdle();
    DestroyCommands();
    DestroyResources();
    
    // Destroy API
    Device.destroyCommandPool(CommandPool);
    Device.destroy();
    Instance.destroySurfaceKHR(Surface);
    Instance.destroy();
}

void VKContext::Load() {
    // Source: https://alain.xyz/blog/raw-vulkan | https://gist.github.com/graphitemaster/e162a24e57379af840d4
    vk::Result result;

    // Instance Extensions
    vector<vk::ExtensionProperties> availableExtensions = vk::enumerateInstanceExtensionProperties();
    vector<const char *> neededExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        #ifdef APP_DEBUG_MODE
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        #endif
        #ifdef VK_USE_PLATFORM_WIN32_KHR
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME

        #endif
    };
    vector<const char *> extensions = GetExtensions(availableExtensions, neededExtensions);

    // Instance Layers
    vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();
    vector<const char *> neededLayers = {
        "VK_LAYER_LUNARG_standard_validation"
    };
    vector<const char *> layers = GetLayers(availableLayers, neededLayers);

    // Application Information
    ApplicationInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = NULL,
        .pApplicationName   = "App",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 0),
        .pEngineName        = "Engine",
        .engineVersion      = VK_MAKE_VERSION(0, 0, 0),
        .apiVersion         = VK_API_VERSION_1_2,
    };

    // Instance Information
    InstanceCreateInfo = {
        .sType                      = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                      = NULL,
        .flags                      = 0,
        //.pApplicationInfo           = &ApplicationInfo,
        .enabledLayerCount          = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames        = layers.data(),
        .enabledExtensionCount      = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames    = extensions.data(),
    };
    InstanceCreateInfo.pApplicationInfo = &ApplicationInfo;

    // Create Instance
    Instance = vk::createInstance(InstanceCreateInfo);

    // Physical Device
    vector<vk::PhysicalDevice> physicalDevices = Instance.enumeratePhysicalDevices();
    PhysicalDevice = physicalDevices[0];

    // Queue Family
    QueueFamilyIndex = GetQueueIndex(PhysicalDevice, vk::QueueFlagBits::eGraphics);

    // Surface
    VkSurfaceKHR surface;
    VkResult resultN = VK_RESULT_MAX_ENUM;
    VkWin32SurfaceCreateInfoKHR info;
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.pNext = NULL;
    info.flags = 0;
    info.hinstance = GetModuleHandle(NULL);
    info.hwnd = WindowHandle;
    resultN = vkCreateWin32SurfaceKHR(static_cast<VkInstance>(Instance), &info, NULL, &surface);
    Surface = surface;

    // Queue Creation
    vk::DeviceQueueCreateInfo qcinfo;
    qcinfo.setQueueFamilyIndex(QueueFamilyIndex);
    qcinfo.setQueueCount(1);
    QueuePriority = 0.5f;
    qcinfo.setPQueuePriorities(&QueuePriority);

    // Device
    vector<vk::ExtensionProperties> availableDeviceExtensions = PhysicalDevice.enumerateDeviceExtensionProperties();
    vector<const char*> neededDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    std::vector<const char*> deviceExtensions = GetExtensions(availableDeviceExtensions, neededDeviceExtensions);
    vk::DeviceCreateInfo dinfo;
    dinfo.setPQueueCreateInfos(&qcinfo);
    dinfo.setQueueCreateInfoCount(1);
    dinfo.setPpEnabledExtensionNames(deviceExtensions.data());
    dinfo.setEnabledExtensionCount(static_cast<uint32_t>(deviceExtensions.size()));
    Device = PhysicalDevice.createDevice(dinfo);

    // Queue
    Queue = Device.getQueue(QueueFamilyIndex, 0);

    // Command Pool
    CommandPool = Device.createCommandPool(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer), QueueFamilyIndex));

    // Surface Attachment Formats
    vector<vk::SurfaceFormatKHR> surfaceFormats = PhysicalDevice.getSurfaceFormatsKHR(Surface);
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
        vk::FormatProperties depthFormatProperties = PhysicalDevice.getFormatProperties(format);

        // Format must support depth stencil attachment for optimal tiling
        if (depthFormatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            SurfaceDepthFormat = format;
            break;
        }
    }

    // Swapchain
    SetupSwapChain();

    // Command Buffers
    CreateCommands();

    // Sync
    CreateSynchronization();

    LoadResources();
    SetupCommands();
}


void VKContext::LoadResources() {
    /**
    * Create Shader uniform binding data structures:
    */

    //Descriptor Pool
    vector<vk::DescriptorPoolSize> descriptorPoolSizes = {
        vk::DescriptorPoolSize(
            vk::DescriptorType::eUniformBuffer,
            1
        )
    };

    DescriptorPool = Device.createDescriptorPool(
        vk::DescriptorPoolCreateInfo(
        vk::DescriptorPoolCreateFlags(),
        1,
        static_cast<uint32_t>(descriptorPoolSizes.size()),
        descriptorPoolSizes.data()
    )
    );

    //Descriptor Set Layout
    // Binding 0: Uniform buffer (Vertex shader)
    vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
        vk::DescriptorSetLayoutBinding(
            0,
        vk::DescriptorType::eUniformBuffer,
        1,
        vk::ShaderStageFlagBits::eVertex,
        nullptr
        )
    };

    DescriptorSetLayouts = {
        Device.createDescriptorSetLayout(
            vk::DescriptorSetLayoutCreateInfo(
        vk::DescriptorSetLayoutCreateFlags(),
        static_cast<uint32_t>(descriptorSetLayoutBindings.size()),
        descriptorSetLayoutBindings.data()
        )
        )
    };

    DescriptorSets = Device.allocateDescriptorSets(
        vk::DescriptorSetAllocateInfo(
        DescriptorPool,
        static_cast<uint32_t>(DescriptorSetLayouts.size()),
        DescriptorSetLayouts.data()
    )
    );

    PipelineLayout = Device.createPipelineLayout(
        vk::PipelineLayoutCreateInfo(
        vk::PipelineLayoutCreateFlags(),
        static_cast<uint32_t>(DescriptorSetLayouts.size()),
        DescriptorSetLayouts.data(),
        0,
        nullptr
    )
    );

    // Setup vertices data
    uint32_t vertexBufferSize = static_cast<uint32_t>(3) * sizeof(Vertex);

    // Setup mIndices data
    Indices.count = 3;
    uint32_t indexBufferSize = Indices.count * sizeof(uint32_t);

    void *data;
    // Static data like vertex and index buffer should be stored on the device memory 
    // for optimal (and fastest) access by the GPU
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
    stagingBuffers.vertices.buffer = Device.createBuffer(
        vk::BufferCreateInfo(
        vk::BufferCreateFlags(),
        vertexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::SharingMode::eExclusive,
        1,
        &QueueFamilyIndex
    )
    );

    auto memReqs = Device.getBufferMemoryRequirements(stagingBuffers.vertices.buffer);

    // Request a host visible memory type that can be used to copy our data do
    // Also request it to be coherent, so that writes are visible to the GPU right after unmapping the buffer
    stagingBuffers.vertices.memory = Device.allocateMemory(
        vk::MemoryAllocateInfo(
        memReqs.size,
        GetMemoryTypeIndex(PhysicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
    )
    );

    // Map and copy
    data = Device.mapMemory(stagingBuffers.vertices.memory, 0, memReqs.size, vk::MemoryMapFlags());
    memcpy(data, VertexBufferData, vertexBufferSize);
    Device.unmapMemory(stagingBuffers.vertices.memory);
    Device.bindBufferMemory(stagingBuffers.vertices.buffer, stagingBuffers.vertices.memory, 0);

    // Create a device local buffer to which the (host local) vertex data will be copied and which will be used for rendering
    Vertices.buffer = Device.createBuffer(
        vk::BufferCreateInfo(
        vk::BufferCreateFlags(),
        vertexBufferSize,
        vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::SharingMode::eExclusive,
        1,
        &QueueFamilyIndex
    )
    );

    memReqs = Device.getBufferMemoryRequirements(Vertices.buffer);

    Vertices.memory = Device.allocateMemory(
        vk::MemoryAllocateInfo(
        memReqs.size,
        GetMemoryTypeIndex(PhysicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
    )
    );

    Device.bindBufferMemory(Vertices.buffer, Vertices.memory, 0);

    // Index buffer
    // Copy index data to a buffer visible to the host (staging buffer)
    stagingBuffers.indices.buffer = Device.createBuffer(
        vk::BufferCreateInfo(
        vk::BufferCreateFlags(),
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::SharingMode::eExclusive,
        1,
        &QueueFamilyIndex
    )
    );
    memReqs = Device.getBufferMemoryRequirements(stagingBuffers.indices.buffer);
    stagingBuffers.indices.memory = Device.allocateMemory(
        vk::MemoryAllocateInfo(
        memReqs.size,
        GetMemoryTypeIndex(PhysicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
    )
    );

    data = Device.mapMemory(stagingBuffers.indices.memory, 0, indexBufferSize, vk::MemoryMapFlags());
    memcpy(data, IndexBufferData, indexBufferSize);
    Device.unmapMemory(stagingBuffers.indices.memory);
    Device.bindBufferMemory(stagingBuffers.indices.buffer, stagingBuffers.indices.memory, 0);

    // Create destination buffer with device only visibility
    Indices.buffer = Device.createBuffer(
        vk::BufferCreateInfo(
        vk::BufferCreateFlags(),
        indexBufferSize,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        vk::SharingMode::eExclusive,
        0,
        nullptr
    )
    );

    memReqs = Device.getBufferMemoryRequirements(Indices.buffer);
    Indices.memory = Device.allocateMemory(
        vk::MemoryAllocateInfo(
        memReqs.size,
        GetMemoryTypeIndex(PhysicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal
    )
    )
    );

    Device.bindBufferMemory(Indices.buffer, Indices.memory, 0);

    auto getCommandBuffer = [&](bool begin) {
        vk::CommandBuffer cmdBuffer = Device.allocateCommandBuffers (
            vk::CommandBufferAllocateInfo(
            CommandPool,
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
        vk::Fence fence = Device.createFence(vk::FenceCreateInfo());

        // Submit to the queue
        Queue.submit(submitInfos, fence);
        // Wait for the fence to signal that command buffer has finished executing
        Device.waitForFences(1, &fence, VK_TRUE, UINT_MAX);
        Device.destroyFence(fence);
        Device.freeCommandBuffers(CommandPool, 1, &commandBuffer);
    };

    flushCommandBuffer(copyCmd);

    // Destroy staging buffers
    // Note: Staging buffer must not be deleted before the copies have been submitted and executed
    Device.destroyBuffer(stagingBuffers.vertices.buffer);
    Device.freeMemory(stagingBuffers.vertices.memory);
    Device.destroyBuffer(stagingBuffers.indices.buffer);
    Device.freeMemory(stagingBuffers.indices.memory);


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
    UniformDataVS.buffer = Device.createBuffer(
        vk::BufferCreateInfo(
            vk::BufferCreateFlags(),
            sizeof(uboVS),
            vk::BufferUsageFlagBits::eUniformBuffer
        )
    );
    // Get memory requirements including size, alignment and memory type 
    memReqs = Device.getBufferMemoryRequirements(UniformDataVS.buffer);
    allocInfo.allocationSize = memReqs.size;
    // Get the memory type index that supports host visible memory access
    // Most implementations offer multiple memory types and selecting the correct one to allocate memory from is crucial
    // We also want the buffer to be host coherent so we don't have to flush (or sync after every update.
    // Note: This may affect performance so you might not want to do this in a real world application that updates buffers on a regular base
    allocInfo.memoryTypeIndex = GetMemoryTypeIndex(PhysicalDevice, memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    // Allocate memory for the uniform buffer
    UniformDataVS.memory = Device.allocateMemory(allocInfo);
    // Bind memory to buffer
    Device.bindBufferMemory(UniformDataVS.buffer, UniformDataVS.memory, 0);

    // Store information in the uniform's descriptor that is used by the descriptor set
    UniformDataVS.descriptor.buffer = UniformDataVS.buffer;
    UniformDataVS.descriptor.offset = 0;
    UniformDataVS.descriptor.range = sizeof(uboVS);

    // Update Uniforms
    float zoom = -2.5f;

    // Update matrices
    uboVS.projectionMatrix = glm::perspective(45.0f, (float)Viewport.width / (float)Viewport.height, 0.01f, 1024.0f);
    uboVS.viewMatrix = glm::translate(glm::identity<glm::mat4>(), { 0.0f, 0.0f, zoom });
    uboVS.modelMatrix = glm::identity<glm::mat4>();

    // Map uniform buffer and update it
    void *pData;
    pData = Device.mapMemory(UniformDataVS.memory, 0, sizeof(uboVS));
    memcpy(pData, &uboVS, sizeof(uboVS));
    Device.unmapMemory(UniformDataVS.memory);


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

    Device.updateDescriptorSets(descriptorWrites, nullptr);

    // Create Render Pass
    CreateRenderPass();
    LoadFrameBuffer();

    // Create Graphics Pipeline

    string vertShaderCode = ReadFile("Assets/Shaders/triangle.vert.spv");
    string fragShaderCode = ReadFile("Assets/Shaders/triangle.frag.spv");

    VertModule = Device.createShaderModule(
        vk::ShaderModuleCreateInfo(
        vk::ShaderModuleCreateFlags(),
        vertShaderCode.size(),
        (uint32_t*)vertShaderCode.data()
    )
    );

    FragModule = Device.createShaderModule(
        vk::ShaderModuleCreateInfo(
            vk::ShaderModuleCreateFlags(),
            fragShaderCode.size(),
            (uint32_t*)fragShaderCode.data()
        )
    );

    PipelineCache = Device.createPipelineCache(vk::PipelineCacheCreateInfo());

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
        &Viewport,
        1,
        &RenderArea
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

    Pipeline = static_cast<vk::Pipeline>(Device.createGraphicsPipeline(
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
            RenderPass,
            0
        )
    ));
}

void VKContext::DestroyResources() {
    // Vertices
    Device.freeMemory(Vertices.memory);
    Device.destroyBuffer(Vertices.buffer);

    // Index buffer
    Device.freeMemory(Indices.memory);
    Device.destroyBuffer(Indices.buffer);

    // Shader Module
    Device.destroyShaderModule(VertModule);
    Device.destroyShaderModule(FragModule);

    // Render Pass
    Device.destroyRenderPass(RenderPass);

    // Graphics Pipeline
    Device.destroyPipelineCache(PipelineCache);
    Device.destroyPipeline(Pipeline);
    Device.destroyPipelineLayout(PipelineLayout);

    // Descriptor Pool
    Device.destroyDescriptorPool(DescriptorPool);
    for (vk::DescriptorSetLayout &dsl : DescriptorSetLayouts) {
        Device.destroyDescriptorSetLayout(dsl);
    }

    // Uniform block object
    Device.freeMemory(UniformDataVS.memory);
    Device.destroyBuffer(UniformDataVS.buffer);

    // Destroy Framebuffers, Image Views
    DestroyFrameBuffer();
    Device.destroySwapchainKHR(Swapchain);

    // Sync
    Device.destroySemaphore(PresentCompleteSemaphore);
    Device.destroySemaphore(RenderCompleteSemaphore);
    for (vk::Fence &f : WaitFences) {
        Device.destroyFence(f);
    }

}


void VKContext::SetupCommands() {
    vector<vk::ClearValue> clearValues = {
        vk::ClearColorValue(
            std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.0f}),
        vk::ClearDepthStencilValue(1.0f, 0)
    };

    for (size_t i = 0; i < CommandBuffers.size(); ++i)
    {
        vk::CommandBuffer& cmd = CommandBuffers[i];
        cmd.reset(vk::CommandBufferResetFlagBits::eReleaseResources);
        cmd.begin(vk::CommandBufferBeginInfo());
        cmd.beginRenderPass(
            vk::RenderPassBeginInfo(
            RenderPass,
            SwapchainBuffers[i].frameBuffer,
            RenderArea,
            static_cast<uint32_t>(clearValues.size()),
            clearValues.data()),
            vk::SubpassContents::eInline);

        cmd.setViewport(0, 1, &Viewport);

        cmd.setScissor(0, 1, &RenderArea);

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
    CommandBuffers = Device.allocateCommandBuffers(
        vk::CommandBufferAllocateInfo(
        CommandPool,
        vk::CommandBufferLevel::ePrimary,
        static_cast<uint32_t>(SwapchainBuffers.size())
    )
    );
}

void VKContext::DestroyCommands() {
    Device.freeCommandBuffers(CommandPool, CommandBuffers);
}


void VKContext::LoadFrameBuffer() {
    // Create Depth Image Data
    DepthImage = Device.createImage(
        vk::ImageCreateInfo(
            vk::ImageCreateFlags(),
            vk::ImageType::e2D,
            SurfaceDepthFormat,
            vk::Extent3D(SurfaceSize.width, SurfaceSize.height, 1),
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

    vk::MemoryRequirements depthMemoryReq = Device.getImageMemoryRequirements(DepthImage);

    // Search through GPU memory properies to see if this can be device local.

    DepthImageMemory = Device.allocateMemory(
        vk::MemoryAllocateInfo(
        depthMemoryReq.size,
        GetMemoryTypeIndex(PhysicalDevice, depthMemoryReq.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
    )
    );

    Device.bindImageMemory(
        DepthImage,
        DepthImageMemory,
        0
    );

    vk::ImageView depthImageView = Device.createImageView(
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

    std::vector<vk::Image> swapchainImages = Device.getSwapchainImagesKHR(Swapchain);

    for (size_t i = 0; i < swapchainImages.size(); i++)
    {
        SwapchainBuffers[i].image = swapchainImages[i];

        // Color
        SwapchainBuffers[i].views[0] =
            Device.createImageView(
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
        SwapchainBuffers[i].views[1] = depthImageView;

        SwapchainBuffers[i].frameBuffer = Device.createFramebuffer(
            vk::FramebufferCreateInfo(
            vk::FramebufferCreateFlags(),
            RenderPass,
            static_cast<uint32_t>(SwapchainBuffers[i].views.size()),
            SwapchainBuffers[i].views.data(),
            SurfaceSize.width, SurfaceSize.height,
            1
        )
        );
    }
}

void VKContext::DestroyFrameBuffer() {
    Device.freeMemory(DepthImageMemory);
    Device.destroyImage(DepthImage);
    if (!SwapchainBuffers.empty()) {
        Device.destroyImageView(SwapchainBuffers[0].views[1]);
    }
    for (size_t i = 0; i < SwapchainBuffers.size(); i++) {
        Device.destroyImageView(SwapchainBuffers[i].views[0]);
        Device.destroyFramebuffer(SwapchainBuffers[i].frameBuffer);
    }
}


void VKContext::CreateRenderPass() {
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

    RenderPass = Device.createRenderPass(
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

void VKContext::CreateSynchronization() {
    // Semaphore used to ensures that image presentation is complete before starting to submit again
    PresentCompleteSemaphore = Device.createSemaphore(vk::SemaphoreCreateInfo());

    // Semaphore used to ensures that all commands submitted have been finished before submitting the image to the queue
    RenderCompleteSemaphore = Device.createSemaphore(vk::SemaphoreCreateInfo());

    // Fence for command buffer completion
    WaitFences.resize(SwapchainBuffers.size());

    for (size_t i = 0; i < WaitFences.size(); i++) {
        WaitFences[i] = Device.createFence(vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));
    }
}

void VKContext::SetupSwapChain() {
    uint32_t width = 1280;
    uint32_t height = 1024;

    vk::Extent2D swapchainSize = vk::Extent2D(width, height);
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = PhysicalDevice.getSurfaceCapabilitiesKHR(Surface);
    if (!(surfaceCapabilities.currentExtent.width == -1 || surfaceCapabilities.currentExtent.height == -1)) {
        swapchainSize = surfaceCapabilities.currentExtent;
        RenderArea = vk::Rect2D(vk::Offset2D(), swapchainSize);
        Viewport = vk::Viewport(0.0f, 0.0f, static_cast<float>(swapchainSize.width), static_cast<float>(swapchainSize.height), 0, 1.0f);
    }

    // VSync
    std::vector<vk::PresentModeKHR> surfacePresentModes = PhysicalDevice.getSurfacePresentModesKHR(Surface);
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eImmediate;
    for (vk::PresentModeKHR &pm : surfacePresentModes) {
        if (pm == vk::PresentModeKHR::eMailbox) {
            presentMode = vk::PresentModeKHR::eMailbox;
            break;
        }
    }

    // Create Swapchain, Images, Frame Buffers
    Device.waitIdle();
    vk::SwapchainKHR oldSwapchain = Swapchain;

    // ToDo: uint32_t backbufferCount = clamp(surfaceCapabilities.maxImageCount, 1U, 2U);
    uint32_t backbufferCount = surfaceCapabilities.maxImageCount;

    Swapchain = Device.createSwapchainKHR(
        vk::SwapchainCreateInfoKHR(
        vk::SwapchainCreateFlagsKHR(),
        Surface,
        backbufferCount,
        SurfaceColorFormat,
        SurfaceColorSpace,
        swapchainSize,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        1,
        &QueueFamilyIndex,
        vk::SurfaceTransformFlagBitsKHR::eIdentity,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        presentMode,
        VK_TRUE,
        oldSwapchain
    )
    );

    // ToDo: SurfaceSize = vk::Extent2D(clamp(swapchainSize.width, 1U, 8192U), clamp(swapchainSize.height, 1U, 8192U));
    SurfaceSize = vk::Extent2D(swapchainSize.width, swapchainSize.height);
    RenderArea = vk::Rect2D(vk::Offset2D(), SurfaceSize);
    Viewport = vk::Viewport(0.0f, 0.0f, static_cast<float>(SurfaceSize.width), static_cast<float>(SurfaceSize.height), 0, 1.0f);


    // Destroy previous swapchain
    if (oldSwapchain != vk::SwapchainKHR(nullptr))
    {
        Device.destroySwapchainKHR(oldSwapchain);
    }

    // Resize swapchain buffers for use later
    SwapchainBuffers.resize(backbufferCount);
}

void VKContext::Attach() {}
void VKContext::Detach() {}

void *VKContext::GetNativeContext() {
    //IM_ASSERT(info->Instance != VK_NULL_HANDLE);
    //IM_ASSERT(info->PhysicalDevice != VK_NULL_HANDLE);
    //IM_ASSERT(info->Device != VK_NULL_HANDLE);
    //IM_ASSERT(info->Queue != VK_NULL_HANDLE);
    //IM_ASSERT(info->DescriptorPool != VK_NULL_HANDLE);
    //IM_ASSERT(info->MinImageCount >= 2);
    //IM_ASSERT(info->ImageCount >= info->MinImageCount);
    //IM_ASSERT(render_pass != VK_NULL_HANDLE);
    static VkContextData *data = new VkContextData();
    data->Intance = Instance;
    data->PhysicalDevice = PhysicalDevice;
    data->Device = Device;
    data->QueueIndex = QueueFamilyIndex;
    data->Queue = Queue;
    data->PipelineCache = PipelineCache;
    data->DescriptorPool = DescriptorPool;
    data->RenderPass = RenderPass;
    data->Surface = Surface;
    data->Swapchain = Swapchain;
    data->CommandPool = CommandPool;
    data->Semaphore = RenderCompleteSemaphore;
    return (void*)data;
}

bool const VKContext::IsCurrentContext() {
    return nullptr;
}

void VKContext::SetViewport(uint32_t width, uint32_t height, int32_t x, int32_t y) {
    Device.waitIdle();
    DestroyFrameBuffer();
    SetupSwapChain();
    LoadFrameBuffer();
    DestroyCommands();
    CreateCommands();
    SetupCommands();
    Device.waitIdle();

    // Uniforms
    uboVS.projectionMatrix = glm::perspective(45.0f, (float)Viewport.width / (float)Viewport.height, 0.01f, 1024.0f);
}

void VKContext::SwapBuffers() {
    RenderTest(Timestamp());
}

void VKContext::SetVSync(bool activate) {}

void VKContext::RenderTest(Timestamp delta) {
    // Swap backbuffers
    vk::Result result;

    result = Device.acquireNextImageKHR(Swapchain, UINT64_MAX, PresentCompleteSemaphore, nullptr, &CurrentBuffer);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        // Swapchain lost, we'll try again next poll
        SetViewport(SurfaceSize.width, SurfaceSize.height, 0, 0);
        return;

    }
    if (result == vk::Result::eErrorDeviceLost) {
        // driver lost, we'll crash in this case:
        exit(1);
    }

    // Update Uniforms
    uboVS.modelMatrix = glm::rotate(uboVS.modelMatrix, (float)(0.001f *delta), glm::vec3( 0.0f, 1.0f, 0.0f ));
    
    void *pData;
    pData = Device.mapMemory(UniformDataVS.memory, 0, sizeof(uboVS));
    memcpy(pData, &uboVS, sizeof(uboVS));
    Device.unmapMemory(UniformDataVS.memory);

    // Wait for Fences
    Device.waitForFences(1, &WaitFences[CurrentBuffer], VK_TRUE, UINT64_MAX);
    Device.resetFences(1, &WaitFences[CurrentBuffer]);

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
    result = Queue.submit(1, &submitInfo, WaitFences[CurrentBuffer]);

    if (result == vk::Result::eErrorDeviceLost)
    {
        // driver lost, we'll crash in this case:
        exit(1);
    }

    result = Queue.presentKHR(
        vk::PresentInfoKHR(
            1,
            &RenderCompleteSemaphore,
            1,
            &Swapchain,
            &CurrentBuffer,
            nullptr
        )
    );

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        // Swapchain lost, we'll try again next poll
        SetViewport(SurfaceSize.width, SurfaceSize.height, 0, 0);
        return;
    }
}

}
