cmake_minimum_required (VERSION 2.6)
project (Testing)

set(SOURCES
  tuningAdjustment.c
  midi.c
  comparison.c
  pitchStrat.c
  onsetStrat.c
  onsetsds.c
  silenceStrat.c
  fVADsd.c
  HPSDetection.c
  BaNaDetection.c
  findpeaks.c
  findCandidates.c
  candidateSelection.c
  lists.c
  winSampleConv.c
  noteCompilation.c
  melodyextraction.c
  simpleDetFunc.c
  gammatoneFilter.c
  filterBank.c
  testOnset.c
)

SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
SET(CMAKE_INSTALL_RPATH "../libfvad-master/src/.libs")

INCLUDE_DIRECTORIES(
  ../libsamplerate-0.1.9/src
  ../libsndfile-1.0.26/src
  ../fftw-3.3.4/api
  ../libfvad-master/include
)

LINK_DIRECTORIES(
  ../fftw-3.3.4/.libs
  ../libsndfile-1.0.26/src/.libs
  ../libfvad-master/src/.libs
  ../libsamplerate-0.1.9/src/.libs
)

add_executable(extract main.c ${SOURCES})
add_library(melodyextraction SHARED ${SOURCES})
TARGET_LINK_LIBRARIES(extract m fftw3f sndfile fvad samplerate)
