#pragma once

#include "Omnia/Omnia.pch"
#include "Omnia/Core.h"

namespace Omnia { namespace Gfx {

class Context {
public:
	static Scope<Context> Create(void *window);

	virtual void Load() = 0;
	virtual void SwapBuffers() = 0;
};

}}
