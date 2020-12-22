# - Try to find the CHECK libraries
#  Once done this will define
#
#  CHECK_FOUND - system has check
#  CHECK_INCLUDE_DIR - the check include directory
#  CHECK_LIBRARIES - check library
#
#  This configuration file for finding libcheck is originally from
#  the opensync project. The originally was downloaded from here:
#  opensync.org/browser/branches/3rd-party-cmake-modules/modules/FindCheck.cmake
#
#  Copyright (c) 2007 Daniel Gollub <dgollub@suse.de>
#  Copyright (c) 2007 Bjoern Ricks  <b.ricks@fh-osnabrueck.de>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
#  Matthew W Abruzzo later refactored this script. It now additionally defines:
#  Check::Check - A target representing check as an imported library


if (CHECK_INSTALL_DIR)

  message(status "Using override CHECK_INSTALL_DIR to find check")
  set( CHECK_INCLUDE_DIR  "${CHECK_INSTALL_DIR}/include" )
  set( CHECK_INCLUDE_DIRS "${CHECK_INCLUDE_DIR}" )
  find_library(CHECK_LIBRARY NAMES check PATHS "${CHECK_INSTALL_DIR}/lib" )
  find_library(COMPAT_LIBRARY NAMES compat PATHS "${CHECK_INSTALL_DIR}/lib")
  set(CHECK_LIBRARIES "${CHECK_LIBRARY}" "${COMPAT_LIBRARY}" )
else()
  # got inspiration from
  # https://gitlab.kitware.com/cmake/community/-/wikis/doc/tutorials/How-To-Find-Libraries
  # on how to handle the general case

  include( FindPkgConfig OPTIONAL RESULT_VARIABLE IMPORTED_FINDPKGCONFIG)

  if (IMPORTED_FINDPKGCONFIG)
    # use FindPkgConfigs to come up with some guesses based on check.pc settings
    pkg_check_modules(PC_CHECK check QUIET)
    find_path( CHECK_INCLUDE_DIRS check.h
      HINTS ${PC_CHECK_INCLUDEDIR} ${PC_CHECK_INCLUDEDIRS})
    find_library(CHECK_LIBRARY NAMES check
      HINTS ${PC_CHECK_LIBDIR} ${PC_CHECK_LIBDIRS} )
  else()
    find_path( CHECK_INCLUDE_DIRS check.h)
    find_library(CHECK_LIBRARY NAMES check
      HINTS ${PC_CHECK_LIBDIR} ${PC_CHECK_LIBDIRS} )
  endif()
  set(CHECK_LIBRARIES "${CHECK_LIBRARY}")
endif()

# The following module helps us handle the QUIET and REQUIRED arguments. It also
# sets the value of CHECK_FOUND appropriately
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CHECK DEFAULT_MSG
  CHECK_LIBRARIES CHECK_INCLUDE_DIRS)

# Hide advanced variables from CMake GUIs
mark_as_advanced( CHECK_INCLUDE_DIRS CHECK_LIBRARIES CHECK_LIBRARY)

# Finally, let's setup an IMPORTED target called Check::Check

if(CHECK_FOUND AND NOT TARGET Check::Check)
  add_library(Check::Check UNKNOWN IMPORTED GLOBAL)
  set_target_properties(Check::Check PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGE "C"
    IMPORTED_LOCATION "${CHECK_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CHECK_INCLUDE_DIRS}"
    )
  if (COMPAT_LIBRARY)
    target_link_libraries(Check::Check INTERFACE "${COMPAT_LIBRARY}")
  endif()
endif()
