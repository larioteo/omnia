
#include "Omnia/GFX/Context.h"
#include "Platform/GFX/OpenGL/GLContext.h"

namespace Omnia { namespace Gfx {

Scope<Context> Context::Create(void *window) {
	return CreateScope<GLContext>(window);
}

}}
