cmake_minimum_required (VERSION 3.0)
project (MelodyExtraction)

SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/libfvad-master/src/.libs")

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}/libsamplerate-0.1.9/src
  ${CMAKE_CURRENT_SOURCE_DIR}/libsndfile-1.0.26/src
  ${CMAKE_CURRENT_SOURCE_DIR}/fftw-3.3.4/api
  ${CMAKE_CURRENT_SOURCE_DIR}/libfvad-master/include
)

LINK_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}/fftw-3.3.4/.libs
  ${CMAKE_CURRENT_SOURCE_DIR}/libsndfile-1.0.26/src/.libs
  ${CMAKE_CURRENT_SOURCE_DIR}/libfvad-master/src/.libs
  ${CMAKE_CURRENT_SOURCE_DIR}/libsamplerate-0.1.9/src/.libs
)

add_subdirectory(src)
