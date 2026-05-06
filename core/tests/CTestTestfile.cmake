# CMake generated Testfile for 
# Source directory: /home/huyenpk/Projects/FastSurfer_QT/_app/core/tests
# Build directory: /home/huyenpk/Projects/FastSurfer_QT/_app/core/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[run_conformed_step_on_already_conformed_scan_produce_conformed_output]=] "/home/huyenpk/Projects/FastSurfer_QT/_app/core/tests/test_step_conform")
set_tests_properties([=[run_conformed_step_on_already_conformed_scan_produce_conformed_output]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/huyenpk/Projects/FastSurfer_QT/_app/core/tests/CMakeLists.txt;17;add_test;/home/huyenpk/Projects/FastSurfer_QT/_app/core/tests/CMakeLists.txt;0;")
add_test([=[run_conform_step_on_non_conformed_mgz_should_produce_conformed_output]=] "/home/huyenpk/Projects/FastSurfer_QT/_app/core/tests/test_native_reconform_non_conformed")
set_tests_properties([=[run_conform_step_on_non_conformed_mgz_should_produce_conformed_output]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/huyenpk/Projects/FastSurfer_QT/_app/core/tests/CMakeLists.txt;33;add_test;/home/huyenpk/Projects/FastSurfer_QT/_app/core/tests/CMakeLists.txt;0;")
add_test([=[run_conform_step_should_match_python_reference_on_mgz_fixture]=] "/home/huyenpk/Projects/FastSurfer_QT/_app/core/tests/test_python_parity_conform_step")
set_tests_properties([=[run_conform_step_should_match_python_reference_on_mgz_fixture]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/huyenpk/Projects/FastSurfer_QT/_app/core/tests/CMakeLists.txt;49;add_test;/home/huyenpk/Projects/FastSurfer_QT/_app/core/tests/CMakeLists.txt;0;")
