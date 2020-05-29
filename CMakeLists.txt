cmake_minimum_required (VERSION 3.8)
project ("Omnia" LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build/${CMAKE_BUILD_TYPE}")

include(CMakeSources.cmake)
add_library(Omnia SHARED ${OMNIA_HEADERS} ${OMNIA_SOURCES})

set_property(TARGET Omnia PROPERTY FOLDER Library)
