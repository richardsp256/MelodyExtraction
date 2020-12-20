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

cmake_minimum_required (VERSION 3.0) # this has not been rigorously tested
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


SET(CMAKE_C_FLAGS, "${CMAKE_C_FLAGS} -Wall -Wextra -fno-trapping-math -fno-math-errno -fno-signed-zeros")
# this is included to facillitate any level of vectorization
# May want to further restrict this in the future
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -march=native")


###############################################################################
# Optional User-Specified Paths to Libraries
#
# Users can specify paths below by selectively uncommenting the following lines
# and inserting paths between the quotation marks. If the user doesn't
# uncomment below, the build system will automatically search for the
# external libraries in the root directory
###############################################################################

# fftw3
#SET( FFTW3_INC "")
#SET( FFTW3_LIB "")

# libsndfile
#SET( SNDFILE_INC "")
#SET( SNDFILE_LIB "")

# libsamplerate:
#SET( SAMPLERATE_INC "")
#SET( SAMPLERATE_LIB "")

# libfvad:
#SET( FVAD_INC "")
#SET( FVAD_LIB "")


# We have an issue with linking libmelodyextraction.so to test files.
# This problem is solved if we set LD_LIBRARY_PATH to point to
# "${CMAKE_CURRENT_SOURCE_DIR}/src".
# We could in principle solve this problem by setting rpath to point to
# "${CMAKE_CURRENT_SOURCE_DIR}/src" while linking the library to the test file,
# unfortunately, we need to also set the rpath to libfvad.
# According to https://cmake.org/Wiki/CMake_RPATH_handling, it is possible to
# have multiple rpaths, however that seems complicated and I am choosing not
# to work on that right now.
#
# For now, I am compiling both a static library and a shared library
#
# If we want to address this problem in the future, we have 2 options:
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


# Determine the paths to libraries that should have been installed in the local
# directory and have names that depend on version numbers:

# The following function checks if a given variable (passed as PATH_VAR) has
# been initialized and that it's value refers an existing directory. If the
# specified path is not a relative path the value of the variable is modified
# to be an absolute path. If the specified path cannot be found, an error is
# raised.
#
# If the variable has not been initialized, then the function attempts to
# assign it a value matched with the pattern passed to DEFAULT_GLOB. If it is
# not successful an error is raised.
#
# The value passed to DEFAULT_GLOB must actually be a value, and not a variable
function(IDENTIFY_PATH PATH_VAR DEFAULT_GLOB)

  if(${PATH_VAR})
    # PATH_VAR had an initial value. Get the absolute path

    if( NOT IS_ABSOLUTE ${${PATH_VAR}})
      get_filename_component(ABSPATH ${${PATH_VAR}} ABSOLUTE)
    else()
      SET(ABSPATH ${${PATH_VAR}})
    endif()

    # check that the path exists and then ensure that PATH_VAR points to the
    # absolute path
    if (IS_DIRECTORY ${ABSPATH})
      SET(${PATH_VAR} ${ABSPATH} PARENT_SCOPE)
    else()
      message(FATAL_ERROR
	"The path set for ${PATH_VAR}, ${${PATH_VAR}}, doesn't exist")
    endif()

  else ()
    # PATH_VAR is not defined, try to set it by matching DEFAULT_GLOB
    FILE(GLOB NEW_PATH ${DEFAULT_GLOB} ONLYDIR)

    # Check if the file matching was a success:
    if (IS_DIRECTORY ${NEW_PATH})
      # set ${PATH_VAR} equal to the identified path
      SET(${PATH_VAR} ${NEW_PATH} PARENT_SCOPE)

      message(STATUS "${PATH_VAR} not specified, set to: \n\t${NEW_PATH}")

    else()
      # A Path was not matched, raise ERROR
      message(FATAL_ERROR
	"${PATH_VAR} not defined and no directory matches: ${DEFAULT_GLOB}")
    endif()

  endif()

endfunction(IDENTIFY_PATH)


# It would be nice to also check for libraries that are installed correctly on
# a system. Not 100% sure how to do that
message(STATUS "Checking for external library paths")

# fftw3:
IDENTIFY_PATH(FFTW3_INC "${CMAKE_CURRENT_SOURCE_DIR}/fftw-3.*/api")
IDENTIFY_PATH(FFTW3_LIB "${CMAKE_CURRENT_SOURCE_DIR}/fftw-3.*/.libs")

# libsndfile:
IDENTIFY_PATH(SNDFILE_INC "${CMAKE_CURRENT_SOURCE_DIR}/libsndfile-1.*/src")
IDENTIFY_PATH(SNDFILE_LIB
  "${CMAKE_CURRENT_SOURCE_DIR}/libsndfile-1.*/src/.libs")

# libsamplerate:
IDENTIFY_PATH( SAMPLERATE_INC
  "${CMAKE_CURRENT_SOURCE_DIR}/libsamplerate-0.*/src" )
IDENTIFY_PATH( SAMPLERATE_LIB
  "${CMAKE_CURRENT_SOURCE_DIR}/libsamplerate-0.*/src/.libs" )

# libfvad:
IDENTIFY_PATH( FVAD_INC
  "${CMAKE_CURRENT_SOURCE_DIR}/libfvad-master/include" )
IDENTIFY_PATH( FVAD_LIB
  "${CMAKE_CURRENT_SOURCE_DIR}/libfvad-master/src/.libs" )


SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH ${FVAD_LIB})


INCLUDE_DIRECTORIES(
  ${FFTW3_INC}
  ${SNDFILE_INC}
  ${SAMPLERATE_INC}
  ${FVAD_INC}
)

LINK_DIRECTORIES(
  ${FFTW3_LIB}
  ${SNDFILE_LIB}
  ${SAMPLERATE_LIB}
  ${FVAD_LIB}
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
add_test(NAME check_rollSigma COMMAND check_rollSigma)
add_test(NAME check_lists COMMAND check_lists)
add_test(NAME check_vector COMMAND check_vector)
add_test(NAME check_calcSummedLagCorrentrograms COMMAND check_vector)
