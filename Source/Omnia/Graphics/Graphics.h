#pragma once

#ifndef APP_UI_GRAPHICS_H
#define APP_UI_GRAPHICS_H

#define APP_GFX_API_OPENGL

#if defined(APP_GFX_API_OPENGL)
    #include "Platform/Graphics/OpenGL/OpenGL.h"
#elif defined(APP_GFX_API_DIRECTX11) && defined(APP_PLATFORM_WINDOWS)
	#include "Platform/Graphics/DirectX/DirectX11.h"
#elif defined(APP_GFX_API_DIRECTX12) && defined(APP_PLATFORM_WINDOWS)
	#include "Platform/Graphics/DirectX/DirectX12.h"
#elif defined(APP_GFX_API_VULKAN)
	#include "Platform/Graphics/Vulkan/Vulkan.h"
#elif defined(APP_GFX_API_XGFX_METAL) && defined(APP_PLATFORM_MACOSX)
	#include "Platform/Graphics/Vulkan/Metal.h"
#elif defined(APP_GFX_API_NULL)
	#include "Platform/Graphics/Null/Null.h"
#else
	#error "No gfx api defined before #include \"CrossWindow/Graphics.h\""
#endif

#endif
