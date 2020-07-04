#include "Context.h"

#include "Omnia/Log.h"

#ifdef APP_PLATFORM_WINDOWS
	#include "Platform/GFX/OpenGL/GLContext.h"
#endif

namespace Omnia {

Reference<Context> Context::Create(void *window) {
	#ifdef APP_PLATFORM_WINDOWS
	switch (API) {
		case GraphicsAPI::OpenGL: {
			return CreateReference<GLContext>(window);
		}

		default: {
			AppAssert(false, "This API is currently not supportded!");
			return nullptr;
		}
	}
	#else
		APP_ASSERT(false, "This platform is currently not supported!");
		return nullptr;
	#endif
}

void Context::Destroy() {
}

}
