###############################################################################
# This file is taken and modified from the "money example" of the Check
# tutorial.
# Need to figure out the copyright notice to include in this file in order to
# release the library.
###############################################################################

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

# If pkg-config is not installed on the system, then the
# CHECK_INSTALL_DIR variable must be set to the install
# location of Check. For example, on Windows, this may
# be: C:/Program Files/check
# set(CHECK_INSTALL_DIR "C:/Program Files/check")

find_package(Check REQUIRED)
include_directories(
  ${CHECK_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/../libsamplerate-0.1.9/src
  )
link_directories(
  ${CHECK_LIBRARY_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/../src)

#set(TEST_SOURCES
#  ../src/gammatoneFilter.c
#)
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY LINK_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()

set(TEST_SOURCES
  check_gammatone.c
)

add_executable(check_gammatone ${TEST_SOURCES})
target_link_libraries(check_gammatone m melodyextraction ${CHECK_LIBRARIES} fftw3f sndfile samplerate fvad)
