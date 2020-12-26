/// @file     candidateSelection.h
/// @brief    Declaration of interface functions used for selecting fundamental
///           frequencies from BaNa candidates

#include "../lists.h"

float costFunction(struct distinctCandidate cand1,
		   struct distinctCandidate cand2);

/// Selects the best candidates for fundamental frequency for each window
///
/// @param[in]  windowList List of candidate lists for each frame
/// @param[in]  length The number of contiguous frames for which the candidates
///             should be selected.
/// @param[out] fundamentals An array where the selected fundamental
///             frequencies will be stored.
void candidateSelection(distinctList **windowList, long length,
			float* fundamentals);
void candidateSelectionSegment(float* fundamentals, distinctList **windowList,
			       long final, long start);
