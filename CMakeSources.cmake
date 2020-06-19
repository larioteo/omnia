set (OMNIA_HEADERS "")
set (OMNIA_SOURCES "")

# Default Extensions
set(OMNIA_DEFAULT_HEADERS
	"Source/Omnia.h"
	"Source/Omnia/Core.h"
	"Source/Omnia/Application.h"
	"Source/Omnia/EntryPoint.h"
	"Source/Omnia/Internal.h"
	"Source/Omnia/Layer.h"
	"Source/Omnia/LayerStack.h"
	"Source/Omnia/Platform.h"
	"Source/Omnia/Types.h"
)
list(APPEND OMNIA_HEADERS ${OMNIA_DEFAULT_HEADERS})

set(OMNIA_DEFAULT_SOURCES
	"Source/Omnia.cpp"
	"Source/Omnia/Application.cpp"
	"Source/Omnia/Layer.cpp"
	"Source/Omnia/LayerStack.cpp"
)
list(APPEND OMNIA_SOURCES ${OMNIA_DEFAULT_SOURCES})


# Prime Extensions
set(OMNIA_LIBRARY_PRIME_HEADERS
	"Source/Omnia/Config.h"
	"Source/Omnia/Log.h"
)
list(APPEND OMNIA_HEADERS ${OMNIA_LIBRARY_PRIME_HEADERS})

set(OMNIA_LIBRARY_PRIME_SOURCES
	"Source/Omnia/Config.cpp"
)
list(APPEND OMNIA_SOURCES ${OMNIA_LIBRARY_PRIME_SOURCES})


# Debug Extensions
set(OMNIA_LIBRARY_DEBUG_HEADERS
	"Source/Omnia/Debug/Instrumentor.h"
	"Source/Omnia/Debug/Memory.h"
)
list(APPEND OMNIA_HEADERS ${OMNIA_LIBRARY_DEBUG_HEADERS})

set(OMNIA_LIBRARY_DEBUG_SOURCES
	""
)
list(APPEND OMNIA_SOURCES ${OMNIA_LIBRARY_DEBUG_SOURCES})


# Graphic Extensions
set(OMNIA_LIBRARY_GRAPHICS_HEADERS
	"Source/Omnia/Graphics/Context.h"
	"Source/Platform/Graphics/OpenGL/GLContext.h"

	"Source/Omnia/Graphics/Graphics.h"
	"Source/Platform/Graphics/OpenGL/OpenGL.h"
)
list(APPEND OMNIA_HEADERS ${OMNIA_LIBRARY_GRAPHICS_HEADERS})

set(OMNIA_LIBRARY_GRAPHICS_SOURCES
	"Source/Omnia/Graphics/Context.cpp"
	"Source/Platform/Graphics/OpenGL/GLContext.cpp"

	"Source/Platform/Graphics/OpenGL/OpenGL.cpp"
)
list(APPEND OMNIA_SOURCES ${OMNIA_LIBRARY_GRAPHICS_SOURCES})


# System Extensions
set(OMNIA_LIBRARY_SYSTEM_HEADERS
	"Source/Omnia/System/Cli.h"
)
list(APPEND OMNIA_HEADERS ${OMNIA_LIBRARY_SYSTEM_HEADERS})

set(OMNIA_LIBRARY_SYSTEM_SOURCES
	""
)
list(APPEND OMNIA_SOURCES ${OMNIA_LIBRARY_SYSTEM_SOURCES})


# UI Extensions
set(OMNIA_LIBRARY_UI_HEADERS
	"Source/Omnia/UI/GuiBuilder.h"
	"Source/Omnia/UI/GuiLayer.h"

	"Source/Omnia/UI/EventData.h"
	"Source/Omnia/UI/Event.h"
	"Source/Omnia/UI/Input.h"
	"Source/Omnia/UI/WindowData.h"
	"Source/Omnia/UI/Window.h"
	"Source/Platform/UI/WinAPI/WinEvent.h"
	"Source/Platform/UI/WinAPI/WinInput.h"
	"Source/Platform/UI/WinAPI/WinWindow.h"
)
list(APPEND OMNIA_HEADERS ${OMNIA_LIBRARY_UI_HEADERS})

set(OMNIA_LIBRARY_UI_SOURCES
	"Source/Omnia/UI/GuiBuilder.cpp"
	"Source/Omnia/UI/GuiLayer.cpp"

	"Source/Omnia/UI/Event.cpp"
	"Source/Omnia/UI/Input.cpp"
	"Source/Omnia/UI/Window.cpp"
	"Source/Platform/UI/WinAPI/WinEvent.cpp"
	"Source/Platform/UI/WinAPI/WinInput.cpp"
	"Source/Platform/UI/WinAPI/WinWindow.cpp"
)
list(APPEND OMNIA_SOURCES ${OMNIA_LIBRARY_UI_SOURCES})


# Utility Extensions
set(OMNIA_LIBRARY_UTILITY_HEADERS
	"Source/Omnia/Utility/DateTime.h"
	"Source/Omnia/Utility/Enum.h"
	"Source/Omnia/Utility/Message.h"
	"Source/Omnia/Utility/Property.h"
	"Source/Omnia/Utility/Timer.h"
)
list(APPEND OMNIA_HEADERS ${OMNIA_LIBRARY_UTILITY_HEADERS})
