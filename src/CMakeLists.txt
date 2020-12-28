
# create a simd_vector interface library
# - basically, it is an abstraction for our switchable vector dtype backend
# - it essentially just defines the relevant compiler definitions and options
#   so that whenever any build target is linked to this target, that build
#   target will always be built with these flags
add_library(simd_vector INTERFACE)

if(DEFINED ENV{MELEX_VECTOR_BACKEND})
  if($ENV{MELEX_VECTOR_BACKEND} MATCHES "sse")
    message(STATUS "detected MELEX_VECTOR_BACKEND=sse")
    target_compile_definitions(simd_vector INTERFACE -DUSE_SSE_INTRINSICS)
    target_compile_options(simd_vector INTERFACE "-mssse3")
  elseif($ENV{MELEX_VECTOR_BACKEND} MATCHES "scalar")
    message(STATUS "detected MELEX_VECTOR_BACKEND=scalar")
  else()
    message(FATAL_ERROR "MELEX_VECTOR_BACKEND was set to an unknown value: " $ENV{MELEX_VECTOR_BACKEND})
  endif()
else()
  message(WARNING "MELEX_VECTOR_BACKEND environment variable not detected. Defaulting to scalar backend")
endif()


# Now, we move onto the main build targets

# Search for dependencies
find_package(FFTW3 REQUIRED)
find_package(SndFile REQUIRED)
find_package(Samplerate REQUIRED)
find_package(FVAD REQUIRED)

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
  PUBLIC m FFTW3::Float
  PRIVATE simd_vector SndFile::SndFile Samplerate::Samplerate FVAD::FVAD
  )
target_compile_options(melodyextraction PRIVATE "-Wall")

add_executable(extract main.c)
target_link_libraries(extract melodyextraction)
target_compile_options(extract PRIVATE "-Wall")


install(TARGETS extract melodyextraction
  RUNTIME DESTINATION ${CMAKE_SOURCE_DIR}/bin
)

# In the future, we should ensure proper installation of libmelodyextraction.so
# As a shorter term workaround, we manually copy the library into a newly
# created .libs directory.
add_custom_target(python_dep
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/src/libmelodyextraction.so ${CMAKE_SOURCE_DIR}/.libs/libmelodyextraction.so
  DEPENDS melodyextraction
)
