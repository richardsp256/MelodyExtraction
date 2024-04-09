###############################################################################
# This file is taken and modified from the "money example" of the Check
# tutorial.
# Need to figure out the copyright notice to include in this file in order to
# release the library.
###############################################################################

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)

# This is required for all tests that need to load in data from files
# - currently this is added to all built files. In the future, should just use
#   target_compile_definitions to use this where it's needed. Doing this would
#   involve compiling doubleArrayTesting.c as a standalone library.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DROOT_DIR_PATH=\"${CMAKE_SOURCE_DIR}\"")

# If pkg-config is not installed on the system, then the
# CHECK_INSTALL_DIR variable must be set to the install
# location of Check. For example, on Windows, this may
# be: C:/Program Files/check
# set(CHECK_INSTALL_DIR "C:/Program Files/check")

find_package(Check REQUIRED)
find_package(Threads REQUIRED)
include_directories(
  ${CHECK_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/../libsamplerate-0.1.9/src
  )
link_directories(
  ${CHECK_LIBRARY_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/../src)

set(ARRAY_TEST_SOURCES
  doubleArrayTesting.c
)

set(GAMMATONE_TEST_SOURCES
  check_gammatone.c
)

set(DETFUNCTION_TEST_SOURCES
  check_detFunction.c
)

set(LISTS_TEST_SOURCES
  check_lists.c
)

set(VECTOR_TEST_SOURCES
  check_vector.c
)

add_executable(check_gammatone ${GAMMATONE_TEST_SOURCES} ${ARRAY_TEST_SOURCES})
add_executable(check_detFunction ${DETFUNCTION_TEST_SOURCES}
  ${ARRAY_TEST_SOURCES})
add_executable(check_lists ${LISTS_TEST_SOURCES})
add_executable(check_vector ${VECTOR_TEST_SOURCES})

target_link_libraries(check_gammatone m melodyextraction_static ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} fftw3f sndfile samplerate fvad)
target_link_libraries(check_detFunction m melodyextraction_static ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} fftw3f sndfile samplerate fvad)
target_link_libraries(check_lists m melodyextraction_static ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} fftw3f sndfile samplerate fvad)
target_link_libraries(check_vector m ${CHECK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
