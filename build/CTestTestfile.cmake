# CMake generated Testfile for 
# Source directory: /home/paullopez/cpp-workspace/bermudan_swaption_pricer
# Build directory: /home/paullopez/cpp-workspace/bermudan_swaption_pricer/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[all_tests]=] "/home/paullopez/cpp-workspace/bermudan_swaption_pricer/build/tests")
set_tests_properties([=[all_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/home/paullopez/cpp-workspace/bermudan_swaption_pricer/CMakeLists.txt;102;add_test;/home/paullopez/cpp-workspace/bermudan_swaption_pricer/CMakeLists.txt;0;")
subdirs("_deps/googletest-build")
subdirs("_deps/pybind11-build")
