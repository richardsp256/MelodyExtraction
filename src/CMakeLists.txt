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
  resample.c
  io_wav.c
)



add_executable(extract main.c ${SOURCES})
add_library(melodyextraction SHARED ${SOURCES})
add_library(melodyextraction_static STATIC ${SOURCES})
TARGET_LINK_LIBRARIES(extract m fftw3f sndfile fvad samplerate)
TARGET_LINK_LIBRARIES(melodyextraction m fftw3f sndfile fvad samplerate)
TARGET_LINK_LIBRARIES(melodyextraction_static m fftw3f sndfile fvad samplerate)

install(TARGETS extract
  RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/../
  )
