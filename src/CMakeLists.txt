cmake_minimum_required (VERSION 2.6)
project (Testing)

set(SOURCES
  tuningAdjustment.c
  midi.c
  stft.c
  extractMelodyProcedure.c
  silenceStrat.c
  fVADsd.c
  lists.c
  winSampleConv.c
  noteCompilation.c
  melodyextraction.c
  resample.c
  io_wav.c
  pitch/pitchStrat.c
  pitch/BaNaDetection.c
  pitch/candidateSelection.c
  pitch/findCandidates.c
  pitch/findpeaks.c
  pitch/HPSDetection.c
  onset/onsetStrat.c
  onset/onsetsds.c
  onset/simpleDetFunc.c
  onset/gammatoneFilter.c
  onset/filterBank.c
  onset/pairTransientDetection.c
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
