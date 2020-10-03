#include "VKContext.h"

#include "Omnia/Log.h"
#include "Omnia/UI/Window.h"

// Sources: https://alain.xyz/blog/raw-vulkan | https://gist.github.com/graphitemaster/e162a24e57379af840d4

namespace Omnia {

VKContext::VKContext(void *window) {
    mWindowHandle = reinterpret_cast<HWND>(window);
    mInstance = CreateReference<VKInstance>();
    mPhysicalDevice = CreateReference<VKPhysicalDevice>(mInstance);
    mDevice = CreateReference<VKDevice>(mPhysicalDevice);

    // Surface
    vk::Win32SurfaceCreateInfoKHR surfaceCreateinfo = {};
    surfaceCreateinfo.hinstance = GetModuleHandle(NULL);
    surfaceCreateinfo.hwnd = mWindowHandle;
    mSurface = mInstance->Call().createWin32SurfaceKHR(surfaceCreateinfo);
}

VKContext::~VKContext() {
    mDevice->Call().waitIdle();
    mDevice->Call().destroyPipelineCache(mPipelineCache);
    mInstance->Call().destroySurfaceKHR(mSurface);
}

void VKContext::Load() {
    // Swapchain
    mSwapChain = CreateReference<VKSwapChain>(mDevice, mSurface);
    mSwapChain->Create(1281, 1025); // Resize triggered after start, so this should not be needed

    // PipelineCache
    mPipelineCache = mDevice->Call().createPipelineCache(vk::PipelineCacheCreateInfo());

}

void VKContext::Attach() {
}

void VKContext::Detach() {
}

void *VKContext::GetNativeContext() {
    static VkContextData *data = new VkContextData();
    return (void*)data;
}

bool const VKContext::IsCurrentContext() {
    return true;
}

void VKContext::SetViewport(uint32_t width, uint32_t height, int32_t x, int32_t y) {
    mSwapChain->Resize(width, height);
}

void VKContext::SwapBuffers() {
    mDevice->Call().waitIdle();
    //mSwapChain->Present();
    //mSwapChain->Prepare();
    mDevice->Call().waitIdle();
}

void VKContext::SetVSync(bool activate) {
}

}
