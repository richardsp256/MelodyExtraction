#  Try to find libfvad. Once done this will define:
#
#  FVAD_FOUND - system has libfvad
#  FVAD_INCLUDE_DIRS - the libfvad include directory
#  FVAD_LIBRARIES - libfvad library
#  FVAD::FVAD - A target representing libfvad as an imported library
#
#  Note: it might be wise to try and make use of pkg-config

find_path(FVAD_INCLUDE_DIRS NAMES fvad.h
  HINTS ${CMAKE_SOURCE_DIR}/libfvad-master/include )
find_library(FVAD_LIBRARIES NAMES fvad
  HINTS ${CMAKE_SOURCE_DIR}/libfvad-master/src/.libs)

# The following module helps us handle the QUIET and REQUIRED arguments. It also
# sets the value of CHECK_FOUND appropriately
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FVAD DEFAULT_MSG
  FVAD_LIBRARIES FVAD_INCLUDE_DIRS)

# Hide advanced variables from CMake GUIs
mark_as_advanced( FVAD_INCLUDE_DIRS FVAD_LIBRARIES)

if(FVAD_FOUND AND NOT TARGET FVAD::FVAD)
  add_library(FVAD::FVAD UNKNOWN IMPORTED GLOBAL)
  set_target_properties(FVAD::FVAD PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGE "C"
    IMPORTED_LOCATION "${FVAD_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${FVAD_INCLUDE_DIRS}"
    )
endif()
