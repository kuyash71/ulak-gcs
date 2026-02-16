cmake_minimum_required(VERSION 3.20)

project(ulak_gcs VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep executables in the build root so README run commands stay stable.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# Allow turning tests on/off from the top level.
option(ULAK_GCS_BUILD_TESTS "Build ULAK GCS tests" ON)

# App entry point.
add_subdirectory(src/app)

if(ULAK_GCS_BUILD_TESTS)
  # CTest is used for Phase-1 smoke coverage.
  include(CTest)
  if(BUILD_TESTING)
    # Only wire tests when BUILD_TESTING is enabled.
    add_subdirectory(tests)
  endif()
endif()
