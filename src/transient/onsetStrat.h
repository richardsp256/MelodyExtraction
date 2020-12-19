/// @file     transient.h
/// @brief    Declaration the of the transient detection function
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


/// Our transient detection algorithm is inspired



int TransientDetectionStrategy(float** AudioData, int size, int dftBlocksize,
			int samplerate, intList* onsets);

