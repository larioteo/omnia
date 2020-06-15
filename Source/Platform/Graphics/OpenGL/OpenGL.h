#pragma once

#include <glad/glad.h>
#include "Omnia/Utility/Property.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#undef APIENTRY

#include <Windows.h>

namespace Omnia {

class Window;

namespace Gfx {

// Context Data and Properties
struct ContextData {
#if defined(APP_PLATFORM_API_WIN32)
	HWND hWindow;
	HDC hDeviceContext;
	HGLRC hRenderingContext;
#endif
};

struct ContextProperties {
	Property<bool> Compatible = false;
	ArithmeticProperty<short> AlphaDepth {0, 8};
	ArithmeticProperty<short> ColorDepth {0, 32};
	ArithmeticProperty<short> DepthBuffer {0, 24};
	ArithmeticProperty<short> StencilBuffer {0, 8};
	ArithmeticProperty<short> MSAABuffer {0, 1};
	ArithmeticProperty<short> MSAASamples {2, 0, 16};  // Depends on hardware so we need to find a way around the crashing.
	ArithmeticProperty<short> VersionMajor {4, 0, 9};
	ArithmeticProperty<short> VersionMinor {6, 0, 9};
};

// Default
ContextData CreateContext(Omnia::Window *window, ContextProperties &properties);
void DestroyContext(const ContextData &state);

// Accessors
bool GetCurrentContext(const ContextData &state);
bool GetExtensions(int a = 0);

// Modifiers
void SetContext(const ContextData &state);
void SwapBuffers(const ContextData &state);
void RemoveContext(const ContextData &state);

// Settings
void SetVSync(bool activate);

}}
