#include "VKContext.h"

#include "Omnia/Log.h"
#include "Omnia/UI/Window.h"

namespace Omnia {

VKContext::VKContext(void *window) {
    WindowHandle = reinterpret_cast<HWND>(window);
    mInstance = CreateReference<VKInstance>();
    mPhysicalDevice = CreateReference<VKPhysicalDevice>(mInstance->Call());
    mDevice = CreateReference<VKDevice>(mPhysicalDevice);
}

VKContext::~VKContext() {
    mDevice->GetDevice().waitIdle();
    
    // Destroy API
    mDevice->Destroy();
    mInstance->Call().destroySurfaceKHR(Surface);
    mInstance->Call().destroy();
}

void VKContext::Load() {
    // Sources: https://alain.xyz/blog/raw-vulkan | https://gist.github.com/graphitemaster/e162a24e57379af840d4
    vk::Result result;

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
    mSwapChain->CreateRenderPass();
    mSwapChain->LoadFrameBuffer();
    mSwapChain->CreateSynchronization();
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
    data->RenderPass = mSwapChain->RenderPass;
    data->Surface = Surface;
    data->Swapchain = mSwapChain->mSwapchain;
    data->CommandPool = mDevice->mCommandPool;
    data->Semaphore = mSwapChain->RenderCompleteSemaphore;
    return (void*)data;
}

bool const VKContext::IsCurrentContext() {
    return true;
}

void VKContext::SetViewport(uint32_t width, uint32_t height, int32_t x, int32_t y) {
    mDevice->GetDevice().waitIdle();
    mSwapChain->DestroyFrameBuffer();
    mSwapChain->Create(width, height);
    mSwapChain->LoadFrameBuffer();
    mDevice->GetDevice().waitIdle();

    // Uniforms
    //uboVS.projectionMatrix = glm::perspective(45.0f, (float)Viewport.width / (float)Viewport.height, 0.01f, 1024.0f);
}

void VKContext::SwapBuffers() {}

void VKContext::SetVSync(bool activate) {}






}
