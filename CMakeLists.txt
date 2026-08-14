cmake_minimum_required(VERSION 3.16)
project(NeuroNavigationConsole VERSION 0.1.0 LANGUAGES CXX)

# SemVer pre-release label (CMake project() VERSION is numeric-only).
set(NEURONAV_VERSION_FULL "0.1.0-alpha.1")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets OpenGL)

add_library(neuronav_warnings INTERFACE)
target_compile_options(neuronav_warnings INTERFACE
  $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
)

add_executable(neuronav_console
  src/app/main.cpp
  src/app/MainWindow.cpp
  src/app/MainWindow.h
)

target_include_directories(neuronav_console PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(neuronav_console PRIVATE
  NEURONAV_VERSION_FULL="${NEURONAV_VERSION_FULL}"
)

target_link_libraries(neuronav_console PRIVATE
  neuronav_warnings
  Qt5::Widgets
  Qt5::OpenGL
)

enable_testing()
# Unit / offscreen tests land here on later days.
