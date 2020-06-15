
#include "Omnia/Graphics/Context.h"
#include "Platform/Graphics/OpenGL/GLContext.h"

namespace Omnia { namespace Gfx {

Scope<Context> Context::Create(void *window) {
	return CreateScope<GLContext>(window);
}

}}
