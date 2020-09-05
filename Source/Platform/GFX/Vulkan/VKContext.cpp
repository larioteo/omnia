#include "VKContext.h"

#include "Omnia/Log.h"
#include "Omnia/UI/Window.h"

#include <glad/vulkan.h>

namespace Omnia {
VKContext::VKContext(void *window) {}

VKContext::~VKContext() {}

void VKContext::Load() {
    if (!gladGetVulkanContext()) {
        AppLogCritical("[Context: Failed to load Vulkan!");
        return;
    }
}

void VKContext::Attach() {}
void VKContext::Detach() {}

void *VKContext::GetNativeContext() {
    return nullptr;
}

bool const VKContext::IsCurrentContext() {
    return nullptr;
}

void VKContext::SetViewport(uint32_t width, uint32_t height, int32_t x, int32_t y) {}
void VKContext::SwapBuffers() {}
void VKContext::SetVSync(bool activate) {}

}
