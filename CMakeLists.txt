cmake_minimum_required(VERSION 3.25)

project(perdu
		VERSION 0.0.1
		LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(PERDU_BUILD_TESTS    "Build unit tests"      ON)
option(PERDU_BUILD_EXAMPLES "Build example apps"    ON)

include(cmake/CompilerOptions.cmake)
include(cmake/FetchDependencies.cmake)

add_subdirectory(src/core)
add_subdirectory(src/ecs)
add_subdirectory(src/components)
add_subdirectory(src/renderer)
add_subdirectory(src/app)

if(PERDU_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(PERDU_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
