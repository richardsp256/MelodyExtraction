#  Try to find libsamplerate. Once done this will define:
#
#  SAMPLERATE_FOUND - system has libsamplerate
#  SAMPLERATE_INCLUDE_DIRS - the libsamplerate include directory
#  SAMPLERATE_LIBRARIES - libsamplerate library
#  Samplerate::Samplerate - A target representing libsamplerate as an imported
#      library
#
#  Note:
#     - should try to see if the CMake build system in libsamplerate exports
#       information that makes this detection any easier
#     - additionally, it may be wise to try and make use of pkg-config

include(IdentifyPath)
IDENTIFY_PATH( HINT_SAMPLERATE_INC
  "${CMAKE_SOURCE_DIR}/libsamplerate-0.*/src" )
IDENTIFY_PATH( HINT_SAMPLERATE_LIB
  "${CMAKE_SOURCE_DIR}/libsamplerate-0.*/src/.libs" )

find_path(SAMPLERATE_INCLUDE_DIRS NAMES samplerate.h
  HINTS ${HINT_SAMPLERATE_INC} )
find_library(SAMPLERATE_LIBRARIES NAMES samplerate
  HINTS ${HINT_SAMPLERATE_LIB} )

# The following module helps us handle the QUIET and REQUIRED arguments. It also
# sets the value of CHECK_FOUND appropriately
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SAMPLERATE DEFAULT_MSG
  SAMPLERATE_LIBRARIES SAMPLERATE_INCLUDE_DIRS)

# Hide advanced variables from CMake GUIs
mark_as_advanced( SAMPLERATE_INCLUDE_DIRS SAMPLERATE_LIBRARIES)

if(SAMPLERATE_FOUND AND NOT TARGET Samplerate::Samplerate)
  add_library(Samplerate::Samplerate UNKNOWN IMPORTED GLOBAL)
  set_target_properties(Samplerate::Samplerate PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGE "C"
    IMPORTED_LOCATION "${SAMPLERATE_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${SAMPLERATE_INCLUDE_DIRS}"
    )
endif()
