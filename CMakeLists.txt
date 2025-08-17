cmake_minimum_required(VERSION 3.20)

project(bermudan_swaption_pricer
  VERSION 0.1.0
  LANGUAGES CXX
)

# -----------------------------
# Options
# -----------------------------
option(BUILD_TESTING "Build unit tests" ON)
option(ENABLE_WARNINGS "Enable strict compiler warnings" ON)
option(ENABLE_SANITIZERS "Enable ASan/UBSan in debug builds (non-MSVC)" OFF)
option(ENABLE_LTO "Enable Interprocedural Optimization / LTO" OFF)  # default OFF to avoid lto-wrapper warnings

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------------
# C++ Standard
# -----------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Always build with PIC so static libs can link into shared modules (pybind11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# -----------------------------
# LTO / IPO setup
# -----------------------------
include(CheckIPOSupported)
if(ENABLE_LTO)
  check_ipo_supported(RESULT ipo_ok OUTPUT ipo_msg)
  if(ipo_ok)
    message(STATUS "IPO/LTO enabled")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

    # Hint GCC's LTO plugin to run multiple LTRANS in parallel to avoid the serial warning.
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
      # Respect environment override if provided: LTO_JOBS=8 cmake ...
      if(NOT DEFINED LTO_JOBS)
        include(ProcessorCount)
        ProcessorCount(_nproc)
        if(NOT _nproc OR _nproc EQUAL 0)
          set(_nproc 4)
        endif()
        set(LTO_JOBS ${_nproc})
      endif()
      add_link_options(-Wl,-plugin-opt=jobs=${LTO_JOBS})
    endif()
  else()
    message(WARNING "IPO/LTO requested but not supported: ${ipo_msg}")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
  endif()
else()
  # Explicitly disable IPO on all targets unless user opts in
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
endif()

# -----------------------------
# Dependencies: QuantLib
# -----------------------------
# Try CMake config, then pkg-config fallback (common on Ubuntu 1.25)
find_package(QuantLib CONFIG QUIET)
if(NOT QuantLib_FOUND)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(QuantLib REQUIRED IMPORTED_TARGET quantlib)
  add_library(QuantLib::QuantLib INTERFACE IMPORTED)
  target_link_libraries(QuantLib::QuantLib INTERFACE PkgConfig::QuantLib)
endif()

# -----------------------------
# Core library
# -----------------------------
add_library(bermudan_swaption_pricer
  src/YieldCurveBuilder.cpp
  src/SwapBuilder.cpp
  src/SwaptionCalibrator.cpp
  src/BermudanSwaptionPricer.cpp
)
target_include_directories(bermudan_swaption_pricer PUBLIC ${PROJECT_SOURCE_DIR}/include)
if(QuantLib_INCLUDE_DIRS)
  target_include_directories(bermudan_swaption_pricer PRIVATE ${QuantLib_INCLUDE_DIRS})
endif()
target_link_libraries(bermudan_swaption_pricer PUBLIC QuantLib::QuantLib)

if(ENABLE_WARNINGS)
  if(MSVC)
    target_compile_options(bermudan_swaption_pricer PRIVATE /W4 /permissive-)
  else()
    target_compile_options(bermudan_swaption_pricer PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
  endif()
endif()

if(ENABLE_SANITIZERS AND NOT MSVC)
  target_compile_options(bermudan_swaption_pricer PRIVATE -fsanitize=address,undefined)
  target_link_options(bermudan_swaption_pricer PRIVATE -fsanitize=address,undefined)
endif()

# -----------------------------
# Example executable
# -----------------------------
add_executable(bermudan_main main.cpp)
target_link_libraries(bermudan_main PRIVATE bermudan_swaption_pricer)
if(QuantLib_INCLUDE_DIRS)
  target_include_directories(bermudan_main PRIVATE ${QuantLib_INCLUDE_DIRS})
endif()

# -----------------------------
# Tests
# -----------------------------
if(BUILD_TESTING)
  include(CTest)
  enable_testing()

  include(FetchContent)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  FetchContent_MakeAvailable(googletest)

  set(PROJECT_TESTS
    test/test_curve.cpp
    test/test_swap.cpp
    test/test_bermudan.cpp
    test/test_calibration.cpp
  )
  set(EXISTING_TESTS "")
  foreach(f ${PROJECT_TESTS})
    if(EXISTS ${CMAKE_SOURCE_DIR}/${f})
      list(APPEND EXISTING_TESTS ${f})
    endif()
  endforeach()

  if(EXISTING_TESTS)
    add_executable(tests ${EXISTING_TESTS})
    target_link_libraries(tests PRIVATE bermudan_swaption_pricer GTest::gtest GTest::gtest_main)
    if(QuantLib_INCLUDE_DIRS)
      target_include_directories(tests PRIVATE ${QuantLib_INCLUDE_DIRS})
    endif()
    add_test(NAME all_tests COMMAND tests)
  else()
    message(WARNING "No test sources found; skipping test target")
  endif()
endif()

# -----------------------------
# Python bindings with pybind11
# -----------------------------
include(FetchContent)
FetchContent_Declare(
  pybind11
  URL https://github.com/pybind/pybind11/archive/refs/tags/v2.12.0.zip
)
FetchContent_MakeAvailable(pybind11)

pybind11_add_module(bermudan_native bindings/bermudan_bindings.cpp)
target_link_libraries(bermudan_native PRIVATE bermudan_swaption_pricer QuantLib::QuantLib)
if(QuantLib_INCLUDE_DIRS)
  target_include_directories(bermudan_native PRIVATE ${QuantLib_INCLUDE_DIRS})
endif()

# If LTO is globally off, ensure pybind11 module doesn't try to enable it by itself
if(NOT ENABLE_LTO)
  set_target_properties(bermudan_native PROPERTIES INTERPROCEDURAL_OPTIMIZATION FALSE)
endif()

if(APPLE)
  set_target_properties(bermudan_native PROPERTIES MACOSX_RPATH ON)
endif()

# -----------------------------
# Install
# -----------------------------
include(GNUInstallDirs)
install(TARGETS bermudan_swaption_pricer bermudan_main bermudan_native
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

