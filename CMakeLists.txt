cmake_minimum_required (VERSION 3.8)
project ("Omnia" LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Build/${CMAKE_BUILD_TYPE}")

add_library(Omnia SHARED "Source/Omnia.cpp" "Source/Omnia.h")

set_property(TARGET Omnia PROPERTY FOLDER Library)
