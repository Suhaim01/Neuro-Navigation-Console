add_library(nnc_reg
  PairedPointRegistration.cpp
  PairedPointRegistration.h
)

target_include_directories(nnc_reg PUBLIC
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(nnc_reg PUBLIC
  nnc_io
  nnc_warnings
)
