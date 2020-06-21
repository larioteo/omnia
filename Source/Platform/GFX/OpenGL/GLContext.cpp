#include "GLContext.h"

#include "Omnia/Log.h"

#include "OpenGL.h"
#include "Omnia/UI/Window.h"

namespace Omnia { namespace Gfx {

GLContext::GLContext(void *window):
	Data{ nullptr },
	Properties{ nullptr },
	pWindow{ window } {

}

void GLContext::Load() {

	// Register Components
	//*Data = Gfx::CreateContext(Window, Gfx::ContextProperties());
	Gfx::SetContext(*Data);

	// Load GL Library
	if (!gladLoadGL()) {
		applog << "Failed to load OpenGL." << std::endl;
		return;
	}
}

void GLContext::SwapBuffers() {
	Gfx::SwapBuffers(*Data);
}

}}
