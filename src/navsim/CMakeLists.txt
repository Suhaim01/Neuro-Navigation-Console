add_executable(navsim
  main.cpp
)

target_link_libraries(navsim PRIVATE
  nnc_io
  nnc_warnings
)
