#pragma once

#include "Omnia/GFX/Context.h"

namespace Omnia {

class VKContext: public Context {
public:
    VKContext(void *window);	// previous CreateContext
    virtual ~VKContext();		// previous DestroyContext

    virtual void Load() override;		// previous LoadGL

    virtual void Attach() override;
    virtual void Detach() override;

    // Accessors
    virtual void *GetNativeContext() override;
    virtual bool const IsCurrentContext() override;

    // Mutators
    virtual void SetViewport(uint32_t width, uint32_t height, int32_t x = 0, int32_t y = 0) override;
    virtual void SwapBuffers() override;

    // Settings
    virtual void SetVSync(bool activate) override;
};

}
