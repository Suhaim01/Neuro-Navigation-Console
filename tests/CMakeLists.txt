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

add_executable(tst_igtl_parse
  tst_igtl_parse.cpp
)

target_link_libraries(tst_igtl_parse PRIVATE
  nnc_io
  Qt5::Test
)

add_test(NAME tst_REQ_IGTL_TransformParse
         COMMAND $<TARGET_FILE:tst_igtl_parse>)

add_executable(tst_paired_point_svd
  tst_paired_point_svd.cpp
)

target_link_libraries(tst_paired_point_svd PRIVATE
  nnc_reg
  Qt5::Test
)

add_test(NAME tst_REQ_REG_001_PairedPointValidation
         COMMAND $<TARGET_FILE:tst_paired_point_svd>)

add_executable(tst_fiducial_loader
  tst_fiducial_loader.cpp
)

target_compile_definitions(tst_fiducial_loader PRIVATE
  NNC_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures"
)

target_link_libraries(tst_fiducial_loader PRIVATE
  nnc_reg
  Qt5::Test
)

add_test(NAME tst_REQ_REG_002_FiducialLoader
         COMMAND $<TARGET_FILE:tst_fiducial_loader>)

add_executable(tst_tool_composition
  tst_tool_composition.cpp
)

target_link_libraries(tst_tool_composition PRIVATE
  nnc_reg
  Qt5::Test
)

add_test(NAME tst_REQ_FRAME_004_ToolComposition
         COMMAND $<TARGET_FILE:tst_tool_composition>)

add_executable(tst_tool_overlay
  tst_tool_overlay.cpp
)

target_link_libraries(tst_tool_overlay PRIVATE
  nnc_reg
  Qt5::Test
)

add_test(NAME tst_REQ_GUI_001_ToolOverlayImage
         COMMAND $<TARGET_FILE:tst_tool_overlay>)

add_executable(tst_registration_bootstrap
  tst_registration_bootstrap.cpp
  ${CMAKE_SOURCE_DIR}/src/app/SceneModel.cpp
  ${CMAKE_SOURCE_DIR}/src/app/RegistrationBootstrap.cpp
)

target_compile_definitions(tst_registration_bootstrap PRIVATE
  NNC_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures"
)

target_include_directories(tst_registration_bootstrap PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(tst_registration_bootstrap PRIVATE
  nnc_reg
  nnc_io
  Qt5::Test
)

add_test(NAME tst_REQ_REG_002_RegistrationBootstrap
         COMMAND $<TARGET_FILE:tst_registration_bootstrap>)
