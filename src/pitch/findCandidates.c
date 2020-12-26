#include <stdlib.h>
#include <stdio.h>
#include "../lists.h"
#include "findCandidates.h"

static const float ratioRanges[15] = {1.15, 1.29, 1.42, 1.59,
				      1.8,  1.9,  2.1,  2.4,
				      2.6,  2.8,  3.2,  3.8,
				      4.2,  4.8,  5.2};

static const int mRanges[15] = { 4,  3,  2,  3,
				-1,  1, -1,  2,
				-1,  1, -1,  1,
				-1,  1, -1};


/// Helper function that estimates the harmonic order of f_i assuming that f_i and f_j
/// are both members of a harmonic series
///
/// This is operation is discussed in section III.C of Yang, Ba, Cai, Demirkol &
/// Heinzelmann. In the paper, they refer to the computed value as `m`. The highest
/// assumed harmonic order of f_j is 5.
///
/// @params[in] f_i, f_j Two potential harmonic frequencies. f_i should be smaller.
/// @returns The estimated harmonic order. Negative values indicate failure
static inline float calcM(float f_i, float f_j){
	// find the index, i, of the right most value in ratioRanges less than
        // f_j/f_i. it returns mRanges[i]
	float ratio = f_j/f_i;

	// basically using algorithm from python's bisect left and then moving
	// one index to the left. This could probably be improved

	int i = bisectLeft(ratioRanges,ratio,0,15);

	if (ratio > ratioRanges[14]){
		// f_j is greater than a 5th order harmonic.
		return -1;
	} else if (i == 0) { // can't shift an index to the left
		return -2;
	} else {
		return mRanges[i-1];
	}
}

void RatioAnalysisCandidates(const float* peaks, int numPeaks,
			     struct orderedList *candidates)
{
	for (int i=0; i<numPeaks-1;i++){
		for (int j=i+1; j<numPeaks; j++){
			float m = calcM(peaks[i],peaks[j]);
			if (m>0) {
				orderedListInsert(candidates, peaks[i]/m);
			}				
		}
	}
}



distinctList* distinctCandidates(struct orderedList* candidates,
				 int max_length, float xi,
				 float f0Min, float f0Max)
{	
	// Finds the distinct candidates from the list of candidates. This is
        // done by selecting the candidate with the most other candidates
        // within xi Hz

	int * confidence = malloc(sizeof(int) * (candidates->length));
	if (!confidence){
		return NULL;
	}

	distinctList * distinct = (distinctListCreate(max_length));
	if (!distinct){
		free(confidence);
		return NULL;
	}

	int first = -1;

	while (candidates->length > 0){
		// set entries of confidence to 1
		for (int i=0; i<(candidates->length); i++){
			confidence[i] = 1;
		}
		// determine confidence values for each index
		for (int i=0; i < (candidates->length - 1); i++){
			for (int j=i+1; j < candidates->length; j++){
				if ((candidates->array[j] - candidates->array[i])<=xi) { 
					confidence[i]+=1;
					confidence[j]+=1;
				} else {
					break;
				}
			}
		}

		// determine which candidate has highest confidence
		// break ties by choosing the candidate with lower frequency
		int maxConfidence = confidence[0];
		int maxIndex = 0;
		for (int i = 1; i<(candidates->length);i++){
			if (confidence[i]>maxConfidence) {
				maxIndex = i;
				maxConfidence = confidence[i];
			}
		}
		
		// add that candidate to distinct
		if ((candidates->array[maxIndex] >= f0Min) &&
		    (candidates->array[maxIndex] <= f0Max)){
			struct distinctCandidate temp = { candidates->array[maxIndex],
				 confidence[maxIndex], 0.0, -1};
			distinctListAppend(distinct,temp);
		}
		// determine first: index of first entry in candidates that is
		// within xi Hz of the candidate with the highest confidence
		first = maxIndex;
		for (int i=maxIndex-1;i>=0;i--){
			if ((candidates->array[maxIndex] -
			     candidates->array[i])<=xi) {
				first = i;
			} else {
				break;
			}
		}

		// determine last: index of largest entry in candidates that is
		// within xi Hz of the candidate with the highest confidence
		int last = maxIndex;
		for (int i=maxIndex+1;i<(candidates->length);i++){
			if ((candidates->array[i] - candidates->array[maxIndex])<=xi) {
				last = i;
			} else {
				break;
			}
		}

		// remove that candidate and all of the candidates within xi Hz 
		// from that candidate from candidates
		orderedListDeleteEntries(candidates, first, last+1);

	}

	free(confidence);
	return distinct;
}
