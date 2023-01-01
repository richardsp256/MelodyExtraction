#  Try to find libfftw3f. Once done this will define:
#
#  FFTW3_FOUND - system has fftw3
#  FFTW3_INCLUDE_DIRS - the fftw3 include directory
#  FFTW3_LIBRARIES - fftw3 library
#  FFTW3::Float - A target representing the floating point version of fftw3 as
#      an imported library
#
#  Note:
#     - There are a number of FindFFTW3.cmake files floating around on the
#       internet. It could be worth checking to see if we could adapt any of
#       their ideas to improve things
#     - additionally, it may be wise to try and make use of pkg-config

include(IdentifyPath)
IDENTIFY_PATH( HINT_FFTW3_INC
  "${CMAKE_SOURCE_DIR}/fftw-3.*/api" )
IDENTIFY_PATH( HINT_FFTW3_LIB
  "${CMAKE_SOURCE_DIR}/fftw-3.*/.libs" )

find_path(FFTW3_INCLUDE_DIRS NAMES fftw3.h
  HINTS ${HINT_FFTW3_INC} )
find_library(FFTW3_LIBRARIES NAMES fftw3f
  HINTS ${HINT_FFTW3_LIB} )

# The following module helps us handle the QUIET and REQUIRED arguments. It also
# sets the value of CHECK_FOUND appropriately
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFTW3 DEFAULT_MSG
  FFTW3_LIBRARIES FFTW3_INCLUDE_DIRS)

# Hide advanced variables from CMake GUIs
mark_as_advanced( FFTW3_INCLUDE_DIRS FFTW3_LIBRARIES)

if(FFTW3_FOUND AND NOT TARGET FFTW3::Float)
  add_library(FFTW3::Float UNKNOWN IMPORTED GLOBAL)
  set_target_properties(FFTW3::Float PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGE "C"
    IMPORTED_LOCATION "${FFTW3_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${FFTW3_INCLUDE_DIRS}"
    )
endif()
