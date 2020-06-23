#pragma once

#include "Omnia/GFX/Context.h"

namespace Omnia { namespace Gfx {

class GLContext: public Context {
	void *pWindow;

public:
	GLContext(void *window);
	virtual ~GLContext();

	virtual void Load() override;
	void Unload() {}

	void SetViewport(const int32_t x, const int32_t y, const uint32_t width, const uint32_t height) {}
	virtual void SwapBuffers() override;
};

}}
