add_executable(nnc_console
  main.cpp
  MainWindow.cpp
  MainWindow.h
  SceneModel.cpp
  SceneModel.h
  VolumeUploadSurface.cpp
  VolumeUploadSurface.h
)

target_compile_definitions(nnc_console PRIVATE
  NNC_VERSION_FULL="${NNC_VERSION_FULL}"
)

target_link_libraries(nnc_console PRIVATE
  nnc_io
  nnc_reg
  nnc_render
  Qt5::Widgets
  Qt5::OpenGL
)
