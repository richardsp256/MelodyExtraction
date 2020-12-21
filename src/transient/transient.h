/// @file     transient.h
/// @brief    [\ref transient] Declaration of the transient detection function
///
/// This is the main header file for the [\ref transient] module.

/// @defgroup transient Transient Detection Module
/// Defines transient detection functions
///
/// In the transient module, functionallity is implemented to detect transients
/// (note onsets and note offsets) from a recording of monophonic singing.
/// The algorithm is adapted from Chang & Lee (2014).
///
/// The algorithm can be divided into 2 main parts:
/// 1. Detection Function calculation
/// 2. Transient identification from detection function
///
/// The first part of the algorithms computes a "detection function" from the
/// input signal. This requires several components:
///    - A gammatone filterbank to filter the input signal
///    - A rolling standard deviation calculation (to optimize the bandwidth of
///      the kernel function)
///    - Calculation of summed corretrograms
///
/// The second part is much simpler. It simply identifies transients from the
/// detection function.

#include "../lists.h"

// TODO:
// - change name of TransientDetectionStrategy
// - update the signature of TransientDetectionStrategy
//      - change `float** AudioData` to `const float * AudioData`
//      - remove `dftBlocksize`
// - change the name of this file to transient.h
// - Present an overview of the transient detection functionally
// - move sigoptimizer to it it's own file
// - rename pairTransientDetection.[ch] -> selectTransients.[ch] and change the
//   name of detectTransients to selectTransients
// - move pairwiseTransientDetection to transients.c
// - change the name from simpleDetFunc to CalcDetFuncSimple
// - deal with the filterBank file (either update it or remove it!)

/// @ingroup transient
/// Identifies pairs of onsets and offsets from audio data using the algorithm
/// published by Chang & Lee (2016)
///
/// @param[in] audioData The audio data for which the transients should be
///            identified
/// @param[in] size Length of audioData
/// @param[in] samplerate The sample rate of the audio data (in Hz)
/// @param[out] transients A pointer to an initially empty intList. This list
///             will be filled with integers corresponding to the audio frame
///             where transients occured. The transients alternate between
///             onsets and offsets. The first value is always an onset while
///             the last value is always an offset.
///
/// @par Note:
/// This uses the default settings mentioned in the method paper
int DetectTransients(float *audioData, int size, int samplerate,
		     intList* transients);

/// @ingroup transient
/// This is nearly the same as DetectTransients, except the audio is resampled
/// to 11025 Hz. This directly calls DetectTransients
///
/// @par Note:
/// The output transients list is adjusted internally by this function such
/// that it holds the indices of samples (at the original sampling rate) when
/// the transients occur
int DetectTransientsFromResampled(const float* AudioData, int size,
				  int samplerate, intList* transients);
