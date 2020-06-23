#include "GLContext.h"

#include <glad/glad.h>

#if defined(APP_PLATFORM_API_WIN32)
	#pragma comment(lib, "opengl32.lib")

	#define VC_EXTRALEAN
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#undef APIENTRY

	#include <Windows.h>
	//#include <GL/gl.h>
	//#define GL_EXT_color_subtable
	#include <GL/glext.h>
	#include "GL/wglext.h"
#endif

#include "Omnia/Log.h"
#include "Omnia/UI/Window.h"
#include "Omnia/Utility/Property.h"

// Get Extension
#if defined(APP_PLATFORM_API_WIN32)
	bool GetExtensions(int a);
	inline PROC GetExtension(const char *functionName) {
		return wglGetProcAddress(functionName);
	}

	//typedef HGLRC WINAPI wglCreateContextAttribsARB_t(HDC hdc, HGLRC hShareContext, const int *attribList);
	//wglCreateContextAttribsARB_t *wglCreateContextAttribsARB;

	//typedef BOOL WINAPI wglChoosePixelFormatARB_t(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
	//wglChoosePixelFormatARB_t *wglChoosePixelFormatARB;

	//typedef BOOL WINAPI wglSwapIntervalEXT_t(int interval);
	//wglSwapIntervalEXT_t *wglSwapIntervalEXT;

#endif

#if !defined(GL_SR8_EXT)
#define GL_SR8_EXT 0x8FBD
#endif
#if !defined(GL_SRG8_EXT)
#define GL_SRG8_EXT 0x8FBE
#endif
#if !defined(EGL_OPENGL_ES3_BIT)
#define EGL_OPENGL_ES3_BIT 0x0040
#endif

namespace Omnia { namespace Gfx {

GLContext::GLContext(void *window): pWindow{ window } {
}

GLContext::~GLContext() {
}

void GLContext::Load() {
	// Load GL Library
	if (!gladLoadGL()) {
		applog << "Failed to load OpenGL." << std::endl;
		return;
	}
}

void GLContext::SwapBuffers() {
}

}}
