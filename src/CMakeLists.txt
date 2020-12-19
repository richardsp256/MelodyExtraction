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
  errors.c
  pitch/pitchStrat.c
  pitch/BaNaDetection.c
  pitch/candidateSelection.c
  pitch/findCandidates.c
  pitch/findpeaks.c
  pitch/HPSDetection.c
  transient/onsetStrat.c
  transient/simpleDetFunc.c
  transient/calcSummedLagCorrentrograms.c
  transient/gammatoneFilter.c
  transient/filterBank.c
  transient/pairTransientDetection.c
)



add_executable(extract main.c ${SOURCES})
add_library(melodyextraction SHARED ${SOURCES})
add_library(melodyextraction_static STATIC ${SOURCES})
TARGET_LINK_LIBRARIES(extract m fftw3f sndfile fvad samplerate)
TARGET_LINK_LIBRARIES(melodyextraction m fftw3f sndfile fvad samplerate)
TARGET_LINK_LIBRARIES(melodyextraction_static m fftw3f sndfile fvad samplerate)

# place the constructed binary and libraries into a places
# - moving the libraries is necessary for supporting the python bindings for
#   out-of-source builds.
# - in the future, this should be modified located so that the libraries are
#   installed to appropriate locations
install(TARGETS extract melodyextraction melodyextraction_static
  RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin
  LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/.libs
  ARCHIVE DESTINATION ${CMAKE_SOURCE_DIR}/.libs
)
