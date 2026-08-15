add_library(nnc_render
  VolumeTexture.cpp
  VolumeTexture.h
  shaders.qrc
)

target_include_directories(nnc_render PUBLIC
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(nnc_render
  PUBLIC
    nnc_io
    nnc_warnings
    Qt5::Gui
    Qt5::OpenGL
)
