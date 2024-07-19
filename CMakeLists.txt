# Note: our original root CMakeLists.txt file was based on the example one
#   included for use in the Check tutorial. We threw away that file and rewrote
#   it from scratch and then added the lines to choose a default build type
#   back in (which we had added ourselves). Because this file may still be
#   considered a derived work, we include the 3-Clause BSD Liscense from that
#   Check tutorial file in licenses/CHECK_TUTORIAL_CMAKELIST_LICENSE

cmake_minimum_required(VERSION 3.16) # not rigorously tested
project(MelodyExtraction LANGUAGES C)

# The only features used from C11 are:
# - aligned_alloc function
# - the _Alignas specifier
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# if the cmake build type was not specified, fall back to a release build
if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build-type was detecting. Defaulting to Release")
  set(CMAKE_BUILD_TYPE Release)
endif()

# Initialize the module path (so that packages can be found later)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
message("${CMAKE_MODULE_PATH}")

# Add the main source directory. This the following targets:
# - the melodyextraction library
# - the extract executable which is linked to melodyextraction
# - the simd_vector interface library (relevant for testing)
add_subdirectory(src)


find_package(Check) # if the Check library is installed, setup testing
if (CHECK_FOUND)
  enable_testing()
  add_subdirectory(tests)
else()
  message(WARNING "The Check library was not found - Tests can't be built")
endif()
