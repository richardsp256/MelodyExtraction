find_package(Check REQUIRED)

# testtools helper library defines utilities that assist with testing
add_library(testtools STATIC
  testtools/doubleArrayTesting.c
  testtools/genSignal.c
  )
target_include_directories(testtools
  PUBLIC ${CMAKE_CURRENT_LIST_DIR}/testtools
  )
target_compile_definitions(testtools
  PRIVATE -DROOT_DIR_PATH=${CMAKE_SOURCE_DIR}
  )
target_link_libraries(testtools PRIVATE m Check::Check)


# Now we define the actual tests

# first, define the tests that check functionality in the transient module
set(TRANSIENT_TEST_SOURCES
  transient/check_transient_main.c
  transient/check_calcSummedLagCorrentrograms.c
  transient/check_gammatone.c
  transient/check_rollSigma.c
  transient/check_vector.c
  )
add_executable(check_transient ${TRANSIENT_TEST_SOURCES})
target_link_libraries(check_transient
  melodyextraction Check::Check m testtools)

# second, define tests that checks the growable lists
add_executable(check_lists check_lists.c)
target_link_libraries(check_lists melodyextraction Check::Check)
