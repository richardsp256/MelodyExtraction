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
  transient/transient.c
  transient/calcDetFunc.c
  transient/calcSummedLagCorrentrograms.c
  transient/gammatoneFilter.c
  transient/rollSigma.c
  transient/selectTransients.c
)

add_library(melodyextraction SHARED ${SOURCES})
target_link_libraries(melodyextraction
  PUBLIC m fftw3f
  PRIVATE sndfile fvad samplerate
  )

add_executable(extract main.c)
target_link_libraries(extract melodyextraction)

# specifying the locations where the constructed binary and libraries are to be
# placed
# - moving the library is necessary for supporting the python bindings for
#   out-of-source builds.
# - in the future, this should be modified located so that the libraries are
#   installed to appropriate locations
install(TARGETS extract melodyextraction
  RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin
  LIBRARY DESTINATION ${CMAKE_SOURCE_DIR}/.libs
)
