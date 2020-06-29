#include "Context.h"

#ifdef APP_PLATFORM_WINDOWS
	#include "Platform/GFX/OpenGL/GLContext.h"
#endif

namespace Omnia { namespace Gfx {

Scope<Context> Context::Create(void *window) {
	//switch (Renderer::GetAPI())
	//{
	//	case RendererAPI::API::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
	//	case RendererAPI::API::OpenGL:  return CreateScope<OpenGLContext>(static_cast<GLFWwindow*>(window));
	//}

	//AppAssert(nullptr, "[GFX:Context]: ", "Unknown Renderer API!");
	//return nullptr;
	return CreateScope<GLContext>(window);
}

}}
