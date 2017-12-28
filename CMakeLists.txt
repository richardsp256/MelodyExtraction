cmake_minimum_required (VERSION 3.0)
project (MelodyExtraction)


###############################################################################
# The following section is taken and modified from the "money example" of the
# Check tutorial.
# Need to figure out the copyright notice to include in this file in order to
# release the library.
###############################################################################
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


###############################################################################
# Set build features
set(CMAKE_BUILD_TYPE Debug)

###############################################################################
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckTypeSize)

###############################################################################
# Check headers
set(INCLUDES "")
macro(ck_check_include_file header var)
  check_include_files("${INCLUDES};${header}" ${var})
  if(${var})
    set(INCLUDES ${INCLUDES} ${header})
  endif(${var})
endmacro(ck_check_include_file)

ck_check_include_file("stdlib.h" HAVE_STDLIB_H)

###############################################################################
# Check functions
# (Nothing to check for the money example)

###############################################################################
# Check defines
# (Nothing to check for the money example)

###############################################################################
# Check struct members
# (Nothing to check for the money example)

###############################################################################
# Check for integer types
# (The following are used in check.h. Regardless if they are used in
# the project, they will need to be checked in order to use Check).
check_type_size(intmax_t INTMAX_T)
check_type_size(uintmax_t UINTMAX_T)

check_type_size(pid_t PID_T)
if(NOT HAVE_PID_T)
  if(WIN32)
    set(pid_t "int")
  else(WIN32)
    MESSAGE(FATAL_ERROR "pid_t doesn't exist on this platform?")
  endif(WIN32)
endif(NOT HAVE_PID_T)

###############################################################################
# Check libraries

###############################################################################
# Generate "config.h" from "cmake/config.h.cmake"
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})
add_definitions(-DHAVE_CONFIG_H)
set(CONFIG_HEADER ${CMAKE_CURRENT_BINARY_DIR}/config.h)

###############################################################################
# This ends the first section taken and modified from the "money example" of
# the Check tutorial.
###############################################################################

SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/libfvad-master/src/.libs")

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}/libsamplerate-0.1.9/src
  ${CMAKE_CURRENT_SOURCE_DIR}/libsndfile-1.0.26/src
  ${CMAKE_CURRENT_SOURCE_DIR}/fftw-3.3.4/api
  ${CMAKE_CURRENT_SOURCE_DIR}/libfvad-master/include
)

LINK_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}/fftw-3.3.4/.libs
  ${CMAKE_CURRENT_SOURCE_DIR}/libsndfile-1.0.26/src/.libs
  ${CMAKE_CURRENT_SOURCE_DIR}/libfvad-master/src/.libs
  ${CMAKE_CURRENT_SOURCE_DIR}/libsamplerate-0.1.9/src/.libs
)


###############################################################################
# The following section is also taken and modified from the "money example" of
# the Check tutorial.
###############################################################################
add_subdirectory(src)
add_subdirectory(tests)

###############################################################################
# Unit tests
enable_testing()
add_test(NAME check_gammatone COMMAND check_gammatone)
