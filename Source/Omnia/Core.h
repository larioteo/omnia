#pragma once

#define APP_PLATFORM_WINDOWS
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
