#include "Context.h"

#ifdef APP_PLATFORM_WINDOWS
	#include "Platform/GFX/OpenGL/GLContext.h"
#endif

namespace Omnia { namespace Gfx {

Scope<Context> Context::Create(void *window) {
	return CreateScope<GLContext>(window);
}

}}
