#pragma once

#include "Omnia/Core.h"
#include "Omnia/Log.h"
#include "Omnia/UI/Window.h"

#include "OpenGL.h"

#if defined(APP_PLATFORM_API_WIN32)
	#pragma comment(lib, "opengl32.lib")
	#if !defined(OPENGL_VERSION_MAJOR)
		#define OPENGL_VERSION_MAJOR 4
	#endif
	#if !defined(OPENGL_VERSION_MINOR)
		#define OPENGL_VERSION_MINOR 3
	#endif
	#define VC_EXTRALEAN
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#undef APIENTRY

	//#include <Windows.h>
	#include <glad/glad.h>
	//#include <GL/gl.h>
	//#define GL_EXT_color_subtable
	#include <GL/glext.h>
	#include "GL/wglext.h"
#endif

namespace Omnia { namespace Gfx {

// Get Extension
#if defined(APP_PLATFORM_API_WIN32)
bool GetExtensions(int a);
inline PROC GetExtension(const char *functionName) {
	return wglGetProcAddress(functionName);
}

typedef HGLRC WINAPI wglCreateContextAttribsARB_t(HDC hdc, HGLRC hShareContext, const int *attribList);
wglCreateContextAttribsARB_t *wglCreateContextAttribsARB;

typedef BOOL WINAPI wglChoosePixelFormatARB_t(HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
wglChoosePixelFormatARB_t *wglChoosePixelFormatARB;

typedef BOOL WINAPI wglSwapIntervalEXT_t(int interval);
wglSwapIntervalEXT_t *wglSwapIntervalEXT;

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

// Context Data and Properties

// Default
ContextData CreateContext(Omnia::Window *window, ContextProperties &properties) {
	ContextData data;

	// Check Compatibility Flag
	auto compatibility = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
	if (properties.Compatible) compatibility = WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
	// Each version has a different set of majors and minors
	switch (properties.VersionMajor) {
		case 1:
			properties.VersionMinor = {properties.VersionMinor, 0, 5};
			break;
		case 2:
			properties.VersionMinor = {properties.VersionMinor, 0, 1};
			break;
		case 3:
			properties.VersionMinor = {properties.VersionMinor, 0, 3};
			break;
		case 4:
			properties.VersionMinor = {properties.VersionMinor, 0, 6};
			break;
		default:
			properties.VersionMajor = 4;
			properties.VersionMinor = 6;
			applog << Log::Error << "App::UI::GFX::GL: Unknown version specified, using default version!" << "\n";
			break;
	}

	// Platform specific stuff
	if constexpr (AppPlatformAPI == "WinAPI") {
		GetExtensions(0);
		data.hWindow = static_cast<HWND>(window->GetNativeWindow());

		// Get Device Context
		//hDeviceContext and hWindow!!!
		data.hDeviceContext = GetDC(data.hWindow);
		if (!data.hDeviceContext) {
			applog << Log::Error << "App::UI::GFX::GL: Error occured while acquireing device context!" << "\n";
			return data;
		}

		// Drawing Surface Pixel Format: https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-pixelformatdescriptor
		PIXELFORMATDESCRIPTOR pfDrawingSurface = {
			.nSize = sizeof(PIXELFORMATDESCRIPTOR),
			.nVersion = 1,									 // Version
			.dwFlags = PFD_DRAW_TO_WINDOW |					 // The buffer can draw to a window or device surface.
					   PFD_DOUBLEBUFFER |					 // The buffer is double-buffered.
					   PFD_SUPPORT_OPENGL |					 // The buffer supports OpenGL drawing.
					   PFD_SWAP_LAYER_BUFFERS,				 // Indicates whether a device can swap individual layer planes with pixel formats that include double-buffered overlay or underlay planes.
			.iPixelType = PFD_TYPE_RGBA,					 // Each pixel has four components in this order: red, green, blue, and alpha.
			.cColorBits = (BYTE)properties.ColorDepth,		 // Color Depth
			.cRedBits = 0,									 // Specifies the number of red bitplanes in each RGBA color buffer.
			.cRedShift = 0,									 // Specifies the shift count for red bitplanes in each RGBA color buffer.
			.cGreenBits = 0,								 // Specifies the number of green bitplanes in each RGBA color buffer.
			.cGreenShift = 0,								 // Specifies the shift count for green bitplanes in each RGBA color buffer.
			.cBlueBits = 0,									 // Specifies the number of blue bitplanes in each RGBA color buffer.
			.cBlueShift = 0,								 // Specifies the shift count for blue bitplanes in each RGBA color buffer.
			.cAlphaBits = 0,								 // Specifies the number of alpha bitplanes in each RGBA color buffer
			.cAlphaShift = 0,								 // Specifies the shift count for alpha bitplanes in each RGBA color buffer.
			.cAccumBits = 0,								 // Specifies the total number of bitplanes in the accumulation buffer.
			.cAccumRedBits = 0,								 // Specifies the number of red bitplanes in the accumulation buffer.
			.cAccumGreenBits = 0,							 // Specifies the number of green bitplanes in the accumulation buffer.
			.cAccumBlueBits = 0,							 // Specifies the number of blue bitplanes in the accumulation buffer.
			.cAccumAlphaBits = 0,							 // Specifies the number of alpha bitplanes in the accumulation buffer.
			.cDepthBits = (BYTE)properties.DepthBuffer,		 // Depth Buffer (Z-Buffer)
			.cStencilBits = (BYTE)properties.StencilBuffer,	 // Stencil Buffer
			.cAuxBuffers = 0,								 // Auxiliary Buffers
			.iLayerType = PFD_MAIN_PLANE,					 // Ignored. Earlier implementations of OpenGL used this member, but it is no longer used.
			.bReserved = 0,									 // Specifies the number of overlay and underlay planes. Bits 0 through 3 specify up to 15 overlay planes and bits 4 through 7 specify up to 15 underlay planes.
			.dwLayerMask = 0,								 // Ignored. Earlier implementations of OpenGL used this member, but it is no longer used.
			.dwVisibleMask = 0,								 // Specifies the transparent color or index of an underlay plane. When the pixel type is RGBA, dwVisibleMask is a transparent RGB color value.
			.dwDamageMask = 0								 // Ignored. Earlier implementations of OpenGL used this member, but it is no longer used.
		};
		// ToDO: Add legacy option
		PIXELFORMATDESCRIPTOR pfd;
		int pixelFormat = 0;
		UINT maxFormats = 1;
		UINT currentFormats = 2;
		int piAttribIList[] = {
			WGL_DRAW_TO_WINDOW_ARB,
			GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB,
			GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB,
			GL_TRUE,
			WGL_ACCELERATION_ARB,
			WGL_FULL_ACCELERATION_ARB,
			WGL_PIXEL_TYPE_ARB,
			WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB,
			properties.ColorDepth,
			WGL_ALPHA_BITS_ARB,
			properties.AlphaDepth,
			WGL_DEPTH_BITS_ARB,
			properties.DepthBuffer,
			WGL_STENCIL_BITS_ARB,
			properties.StencilBuffer,
			WGL_SAMPLE_BUFFERS_ARB,
			properties.MSAABuffer,
			WGL_SAMPLES_ARB,
			properties.MSAASamples,
			WGL_TRANSPARENT_ARB,
			GL_TRUE,
			0,
		};

		wglChoosePixelFormatARB(data.hDeviceContext, piAttribIList, NULL, maxFormats, &pixelFormat, &currentFormats);
		//pixelFormat = ChoosePixelFormat(data.hDeviceContext, &pfDrawingSurface);
		if (!currentFormats) {
			applog << Log::Error << "App::UI::GFX::GL: Error no suiteable pixel format found!" << "\n";
			return data;
		}

		DescribePixelFormat(data.hDeviceContext, pixelFormat, sizeof(pfd), &pfd);
		if (!SetPixelFormat(data.hDeviceContext, pixelFormat, &pfd)) {
			 applog << Log::Error << "App::UI::GFX::GL: Error setting pixel format failed!"<< "\n";
			return data;
		}

		// Create temporary context
		//HGLRC dummyContext = wglCreateContext(data.hDeviceContext);
		//if (!dummyContext) {
		//	applog << Log::Error << "App::UI::GFX::GL: Error occured while creating dummy context!" << "\n";
		//	return data;
		//}
		//if (!wglMakeCurrent(data.hDeviceContext, dummyContext)) {
		//	applog << Log::Error << "App::UI::GFX::GL: Error occured while activating dummy context!" << "\n";
		//	return data;
		//}
		// Get extensions
		//PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
		//PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = NULL;
		//wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)GetExtension("wglCreateContextAttribsARB");
		//wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)GetExtension("wglChoosePixelFormatARB");
		// Remove temporary context
		//wglMakeCurrent(NULL, NULL);
		//wglDeleteContext(dummyContext);

		// Create GL-Context
		int glContextAttributes[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, properties.VersionMajor, WGL_CONTEXT_MINOR_VERSION_ARB, properties.VersionMinor, WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB, WGL_CONTEXT_PROFILE_MASK_ARB, compatibility, 0};
		data.hRenderingContext = wglCreateContextAttribsARB(data.hDeviceContext, NULL, glContextAttributes);
		if (!data.hRenderingContext) {
			applog << Log::Error << "App::UI::GFX::GL: Error occured while creating GL-Context!" << "\n";
			return data;
		}
	}

	return data;
}

void DestroyContext(const ContextData &state) {
	if constexpr (AppPlatformAPI == "WinAPI") {
		wglDeleteContext(state.hRenderingContext);
	}
}

bool LoadGL() {
	if (!gladLoadGL()) {
		applog << "Failed to load OpenGL." << std::endl;
		return false;
	}
	printf("OpenGL Version %d.%d loaded\n", GLVersion.major, GLVersion.minor);
	return true;
}

// Accessors
bool GetCurrentContext(const ContextData &state) {
	if constexpr (AppPlatformAPI == "WinAPI") {
		return (wglGetCurrentContext() == state.hRenderingContext);
	}
}

bool GetExtensions(int a) {
	if constexpr (AppPlatformAPI == "WinAPI") {
		WNDCLASSA window_class = {
			.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
			.lpfnWndProc = DefWindowProcA,
			.hInstance = GetModuleHandle(0),
			.lpszClassName = "Dummy_WGL_djuasiodwa",
		};

		if (!RegisterClassA(&window_class)) {
		}

		HWND dummy_window = CreateWindowExA(
			0,
			window_class.lpszClassName,
			"Dummy OpenGL Window",
			0,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			window_class.hInstance,
			0);

		if (!dummy_window) {
		}

		HDC dummy_dc = GetDC(dummy_window);

		PIXELFORMATDESCRIPTOR pfd = {
			.nSize = sizeof(pfd),
			.nVersion = 1,
			.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			.iPixelType = PFD_TYPE_RGBA,
			.cColorBits = 32,
			.cDepthBits = 24,
			.cStencilBits = 8,
			.iLayerType = PFD_MAIN_PLANE,
		};

		int pixel_format = ChoosePixelFormat(dummy_dc, &pfd);
		if (!pixel_format) {
		}
		if (!SetPixelFormat(dummy_dc, pixel_format, &pfd)) {
		}

		HGLRC dummy_context = wglCreateContext(dummy_dc);
		if (!dummy_context) {
		}

		if (!wglMakeCurrent(dummy_dc, dummy_context)) {
		}

		wglCreateContextAttribsARB = (wglCreateContextAttribsARB_t *)wglGetProcAddress("wglCreateContextAttribsARB");
		wglChoosePixelFormatARB = (wglChoosePixelFormatARB_t *)wglGetProcAddress("wglChoosePixelFormatARB");
		wglSwapIntervalEXT = (wglSwapIntervalEXT_t *)wglGetProcAddress("wglSwapIntervalEXT");

		wglMakeCurrent(dummy_dc, 0);
		wglDeleteContext(dummy_context);
		ReleaseDC(dummy_window, dummy_dc);
		DestroyWindow(dummy_window);
	}
	return true;
}

// Modifiers
void SetContext(const ContextData &state) {
	if constexpr (AppPlatformAPI == "WinAPI") {
		wglMakeCurrent(state.hDeviceContext, state.hRenderingContext);
	}
}

void SetViewport(const size_t width, const size_t height) {
	glViewport(0, 0, width, height);
}

void SwapBuffers(const ContextData &state) {
	if constexpr (AppPlatformAPI == "WinAPI") {
		SwapBuffers(state.hDeviceContext);
	}
}

void RemoveContext(const ContextData &state) {
	if constexpr (AppPlatformAPI == "WinAPI") {
		wglMakeCurrent(state.hDeviceContext, NULL);
	}
}

// Settings
void SetVSync(bool activate) {
	wglSwapIntervalEXT((BOOL)activate);
}

}}
