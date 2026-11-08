cmake_minimum_required(VERSION 3.25)
project(125AFruityOriginalEngineProbe VERSION 0.1.0 LANGUAGES CXX)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
  message(FATAL_ERROR "The original engine probe must be built as Windows Win32/x86.")
endif()
include(CTest)
add_executable(fruity_probe32 src/legacy/probe32.cpp)
target_compile_features(fruity_probe32 PRIVATE cxx_std_23)
set_target_properties(fruity_probe32 PROPERTIES OUTPUT_NAME "125A-Fruity-Probe32")
if(MSVC)
 target_compile_options(fruity_probe32 PRIVATE /W4 /WX)
endif()
if(BUILD_TESTING)
 add_library(fruity_mock_engine SHARED tests/mock_engine.cpp tests/mock_engine.def)
 target_compile_features(fruity_mock_engine PRIVATE cxx_std_23)
 if(MSVC)
  target_compile_options(fruity_mock_engine PRIVATE /W4 /WX)
 endif()
 add_test(NAME x86_mock_pcm_contract
  COMMAND $<TARGET_FILE:fruity_probe32> $<TARGET_FILE:fruity_mock_engine>
  "\${CMAKE_CURRENT_SOURCE_DIR}/tests/mock-bank.txt"
  "\${CMAKE_CURRENT_BINARY_DIR}/mock-output.wav")
 add_test(NAME x86_missing_asset_is_rejected
  COMMAND $<TARGET_FILE:fruity_probe32> $<TARGET_FILE:fruity_mock_engine>
  "\${CMAKE_CURRENT_SOURCE_DIR}/tests/DOES-NOT-EXIST.sf2"
  "\${CMAKE_CURRENT_BINARY_DIR}/should-not-exist.wav")
 set_tests_properties(x86_missing_asset_is_rejected PROPERTIES WILL_FAIL TRUE)
endif()
