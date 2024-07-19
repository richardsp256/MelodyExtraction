/// @file     findCandidates.h
/// @brief    Declaration of functions used for identifying candidates in the
///           BaNa pitch detection algorithm.

#include "../lists.h"

/// Identifies fundamental frequency candidates through ratio analysis of
/// spectral peaks
///
/// For each unique pair of spectral peaks the frequency ratio of the peaks is
/// computed. From that ratio, the function computes the fundamental frequency
/// of the harmonic series that includes both peaks from the pair. This
/// fundamental frequency is then added to the list of spectral frequencies.
/// This procedure is described in section III.C of Yang, Ba, Cai, Demirkol &
/// Heinzelmann.
///
/// @params[in]  peaks Frequencies of the spectrum peaks
/// @params[in]  numPeaks The number of elements in `peaks`
/// @params[out] candidates Ordered list to which fundamental frequency
///              candidates are appended. This MUST have the capcity for
///              `numPeaks*(numPeaks-1)/2` entries.
void RatioAnalysisCandidates(const float* peaks, int numPeaks,
			     struct orderedList *candidates);

/// Construct a list of distinctive of candidates by consolidating all
/// candidates that are close to each other. Confidence scores are assigned
/// to all distinctive candidates based on the number of close candidates.
///
/// @param[in, out] candidates Sorted list of all frequency candidates. During
///                 this function call, elements are removed from this list
///                 until none remain.
/// @param[in]      max_length The maximum number of elements that can be
///                 included
/// @param[in]      xi The maximum interval (in Hz) separating candidates that
///                 are considered close to each other.
/// @param[in]      f0Min Minimum allowed frequency for distinctive candidates
/// @param[in]      f0Max Maximum allowed frequency for distinctive candidates
distinctList* distinctCandidates(struct orderedList* candidates,
				 int max_length, float xi,
				 float f0Min, float f0Max);
