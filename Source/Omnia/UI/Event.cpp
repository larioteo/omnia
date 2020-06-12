#include "Event.h"

#ifdef APP_PLATFORM_WINDOWS
	#include "Platform/UI/WinAPI/WinEvent.h"
#endif

namespace Omnia {

Scope<EventListener> EventListener::Create() {
	#ifdef APP_PLATFORM_WINDOWS
		return CreateScope<WinEventListener>();
	#else
		static_assert("Platform not supported!");
	#endif
}

}