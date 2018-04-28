# Copyright (c) 2017, Branden Archer
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of this project nor the names of its contributors may
#       be used to endorse or promote products derived from this software without
#       specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Branden Archer BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required (VERSION 3.0)
project (MelodyExtraction)



# Right now we have an issue with linking libmelodyextraction.so to test files.
# This problem is solved if we set LD_LIBRARY_PATH to point to
# "${CMAKE_CURRENT_SOURCE_DIR}/src".
# We could in principle solve this problem by setting rpath to point to
# "${CMAKE_CURRENT_SOURCE_DIR}/src" while linking the library to the test file,
# unfortunately, we need to also set the rpath to libfvad.
# According to https://cmake.org/Wiki/CMake_RPATH_handling, it is possible to
# have multiple rpaths, however that seems complicated and I am choosing not
# to work on that right now.
# For now, I am compiling a static library instead libmelodyextraction.a
# If we want to use shared libraries instead of static libraries (which we
# will need based on our licensing choice), I think we will have 2 options:
# 1. Do what is suggested in https://cmake.org/Wiki/CMake_RPATH_handling for
#    using multiple RPATH
# 2. The problem may go away if we actually install the shared library into the
#    standard shared library directory

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

# set the Wall flag
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

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
add_test(NAME check_detFunction COMMAND check_detFunction)
add_test(NAME check_tripleBuffer COMMAND check_tripleBuffer)
add_test(NAME check_sigOpt COMMAND check_sigOpt)
add_test(NAME check_filterBank COMMAND check_filterBank)
