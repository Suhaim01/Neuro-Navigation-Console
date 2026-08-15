cmake_minimum_required(VERSION 3.16)
project(NeuroNavigationConsole VERSION 0.1.0 LANGUAGES CXX)

# SemVer pre-release label (CMake project() VERSION is numeric-only).
set(NNC_VERSION_FULL "0.1.0-alpha.1")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

find_package(Qt5 REQUIRED COMPONENTS Widgets OpenGL Test)

add_library(nnc_warnings INTERFACE)
target_compile_options(nnc_warnings INTERFACE
  $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
)

add_subdirectory(src/io)
add_subdirectory(src/render)
add_subdirectory(src/app)

enable_testing()
add_subdirectory(tests)
