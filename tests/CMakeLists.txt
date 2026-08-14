add_executable(tst_nifti_loader
  tst_nifti_loader.cpp
)

target_compile_definitions(tst_nifti_loader PRIVATE
  NNC_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures"
)

target_link_libraries(tst_nifti_loader PRIVATE
  nnc_io
  Qt5::Test
)

add_test(NAME tst_REQ_FRAME_001_NiftiSformMatchesKnownMatrix
         COMMAND $<TARGET_FILE:tst_nifti_loader>)
