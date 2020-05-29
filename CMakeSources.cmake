set (OMNIA_HEADERS "")
set (OMNIA_SOURCES "")

set(OMNIA_DEFAULT_HEADERS
	"Source/Omnia.h"
	"Source/Omnia/Core.h"
	"Source/Omnia/Application.h"
)
list(APPEND OMNIA_HEADERS ${OMNIA_DEFAULT_HEADERS})

set(OMNIA_DEFAULT_SOURCES
	"Source/Omnia.cpp"
	"Source/Omnia/Application.cpp"
)
list(APPEND OMNIA_SOURCES ${OMNIA_DEFAULT_SOURCES})
