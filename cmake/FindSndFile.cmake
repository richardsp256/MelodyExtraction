#  Try to find libsndfile. Once done this will define:
#
#  SNDFILE_FOUND - system has libsndfile
#  SNDFILE_INCLUDE_DIRS - the libsndfile include directory
#  SNDFILE_LIBRARIES - libsndfile library
#  SndFile::SndFile - A target representing libsndfile as an imported
#      library
#
#  Note:
#     - the libsamplerate github repository ships with a FindSndFile.cmake
#       file. It would be worth checking to see if we could adapt any of their
#       ideas to improve things
#     - additionally, it may be wise to try and make use of pkg-config

include(IdentifyPath)
IDENTIFY_PATH( HINT_SNDFILE_INC
  "${CMAKE_SOURCE_DIR}/libsndfile-1.*/src" )
IDENTIFY_PATH( HINT_SNDFILE_LIB
  "${CMAKE_SOURCE_DIR}/libsndfile-1.*/src/.libs" )

find_path(SNDFILE_INCLUDE_DIRS NAMES sndfile.h
  HINTS ${HINT_SNDFILE_INC} )
find_library(SNDFILE_LIBRARIES NAMES sndfile
  HINTS ${HINT_SNDFILE_LIB} )

# The following module helps us handle the QUIET and REQUIRED arguments. It also
# sets the value of CHECK_FOUND appropriately
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SNDFILE DEFAULT_MSG
  SNDFILE_LIBRARIES SNDFILE_INCLUDE_DIRS)

# Hide advanced variables from CMake GUIs
mark_as_advanced( SNDFILE_INCLUDE_DIRS SNDFILE_LIBRARIES)

if(SNDFILE_FOUND AND NOT TARGET SndFile::SndFile)
  add_library(SndFile::SndFile UNKNOWN IMPORTED GLOBAL)
  set_target_properties(SndFile::SndFile PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGE "C"
    IMPORTED_LOCATION "${SNDFILE_LIBRARIES}"
    INTERFACE_INCLUDE_DIRECTORIES "${SNDFILE_INCLUDE_DIRS}"
    )
endif()
