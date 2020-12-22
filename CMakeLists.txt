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

cmake_minimum_required (VERSION 3.16) # this has not been rigorously tested
project (MelodyExtraction)

###############################################################################
# Build type
###############################################################################

if (NOT CMAKE_BUILD_TYPE)
  MESSAGE(STATUS "No build-type was detecting. Defaulting to Release")
  set(CMAKE_BUILD_TYPE Release)
endif()

###############################################################################
# C compiler flags
###############################################################################

if(DEFINED ENV{MELEX_VECTOR_BACKEND})
  if($ENV{MELEX_VECTOR_BACKEND} MATCHES "sse")
    MESSAGE(STATUS "detected MELEX_VECTOR_BACKEND=sse")
    add_definitions(-DUSE_SSE_INTRINSICS)
  elseif($ENV{MELEX_VECTOR_BACKEND} MATCHES "scalar")
    MESSAGE(STATUS "detected MELEX_VECTOR_BACKEND=scalar")
  else()
    MESSAGE(FATAL_ERROR "MELEX_VECTOR_BACKEND was set to an unknown value: " $ENV{MELEX_VECTOR_BACKEND})
  endif()
else()
  MESSAGE(WARNING "MELEX_VECTOR_BACKEND environment variable not detected. Defaulting to scalar backend")
endif()

# May want to further restrict this in the future
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -march=native -fno-trapping-math -fno-math-errno -fno-signed-zeros")

###############################################################################
# The following section is taken and modified from the "money example" of the
# Check tutorial.
# Need to figure out the copyright notice to include in this file in order to
# release the library.
###############################################################################
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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

add_subdirectory(src)
add_subdirectory(tests)

###############################################################################
# Unit tests
enable_testing()
add_test(NAME check_lists COMMAND check_lists)
add_test(NAME check_transient COMMAND check_transient)
