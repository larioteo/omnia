#pragma once

#ifndef APP_UI_GRAPHICS_H
#define APP_UI_GRAPHICS_H

#define APP_GFX_API_OPENGL

#if defined(APP_GFX_API_OPENGL)
    #include "Platform/GFX/OpenGL/OpenGL.h"
#elif defined(APP_GFX_API_DIRECTX11) && defined(APP_PLATFORM_WINDOWS)
	#include "Platform/GFX/DirectX/DirectX11.h"
#elif defined(APP_GFX_API_DIRECTX12) && defined(APP_PLATFORM_WINDOWS)
	#include "Platform/GFX/DirectX/DirectX12.h"
#elif defined(APP_GFX_API_VULKAN)
	#include "Platform/GFX/Vulkan/Vulkan.h"
#elif defined(APP_GFX_API_XGFX_METAL) && defined(APP_PLATFORM_MACOSX)
	#include "Platform/GFX/Vulkan/Metal.h"
#elif defined(APP_GFX_API_NULL)
	#include "Platform/GFX/Null/Null.h"
#else
	#error "No gfx api defined before #include \"CrossWindow/Graphics.h\""
#endif

#endif
