add_library(nnc_io
  NiftiLoader.cpp
  NiftiLoader.h
  IgtlParser.cpp
  IgtlParser.h
  IgtlReceiver.cpp
  IgtlReceiver.h
)

target_include_directories(nnc_io PUBLIC
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(nnc_io PUBLIC
  nnc_warnings
  Qt5::Core
)
