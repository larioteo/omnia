#pragma once

#include "Platform.h"
#include "Types.h"

#define APP_SHARED_LIBRARY

#ifdef APP_PLATFORM_WINDOWS
    #ifdef APP_SHARED_LIBRARY
        #define APP_API __declspec(dllexport)
    #else
        #define APP_API __declspec(dllimport)
    #endif
#else
	#error "This library currently supports only Windows!""
#endif

#ifdef _DEBUG
	#if defined(APP_PLATFORM_WINDOWS)
		#define APP_DEBUGBREAK() __debugbreak()
	#elif defined(APP_PLATFORM_LINUX)
		#include <signal.h>
		#define APP_DEBUGBREAK() raise(SIGTRAP)
	#else
		#define APP_DEBUGBREAK()
		#pragma message("#> Core: Platform doesn't support debug break!")
	#endif
	
	//#define APP_ASSERT(x, ...) { if(!(x)) { APP_LOG_ERROR("Assertation Failed: ", __VA_ARGS__); APP_DEBUGBREAK(); } }
#else
	#define APP_DEBUGBREAK()
	#define APP_ASSERT(x, ...)
#endif
