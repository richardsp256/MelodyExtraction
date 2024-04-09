#include <stdlib.h>
#include <stdio.h>
#include "../lists.h"
#include "findCandidates.h"

// https://stackoverflow.com/questions/6514651/declare-large-global-array
static float ratioRanges[15] = {1.15, 1.29, 1.42, 1.59,  
				1.8,  1.9,  2.1,  2.4,
				2.6,  2.8,  3.2,  3.8,
				4.2,  4.8,  5.2};

static float mRanges[15] = { 4,  3,  2,  3,  
			    -1,  1, -1,  2,
			    -1,  1, -1,  1,  
			    -1,  1, -1};


struct orderedList calcCandidates(float* peaks, int numPeaks)
{
	int i,j;
	float m;

	// let T_n represent the nth triangle number. T_n = n(n+1)/2
	// the maximum number of candidates from different combinations of
	// peaks is: T_(numPeaks-1)= (numPeaks)(numPeaks-1)/2
	// In addition to those combinations, the list of candidates also
	// includes the lowest Frequency Peak and the cepstral frequency
	// Thus, the maximum length is: 2+(numPeaks)(numPeaks-1)/2
	struct orderedList candidates = orderedListCreate(2+((numPeaks-1) *
							     (numPeaks)/2));

	for (i=0; i<numPeaks-1;i++){
		for (j=i+1; j<numPeaks; j++){
			m = calcM(peaks[i],peaks[j]);
			if (m>0) {
				orderedListInsert(&candidates, peaks[i]/m);
			}				
		}
	}
	return candidates;
}
 
float calcM(float f_i, float f_j){
	// find the index, i, of the right most value in ratioRanges less than
        // f_j/f_i. it returns mRanges[i]
	float ratio = f_j/f_i;

	// basically using algorithm from python's bisect left and then moving
	// one index to the left. This could probably be improved

	int i = bisectLeft(ratioRanges,ratio,0,15);

	if (i == 0) {
		return -1.;
	}

	i= i - 1;

	return mRanges[i];
}


distinctList* distinctCandidates(struct orderedList* candidates,
				 int max_length, float xi,
				 float f0Min, float f0Max)
{	
	// Finds the distinct candidates from the list of candidates. This is
        // done by selecting the candidate with the most other candidates
        // within xi Hz
	
	int first, last, i,j,maxIndex, *confidence, maxConfidence;
	distinctList *distinct;
	distinct = (distinctListCreate(max_length));

	confidence = malloc(sizeof(int) * (candidates->length));

	first = -1;

	while (candidates->length !=0){	        
		// set entries of confidence to 1
		for (i=0;i<(candidates->length);i++){
			confidence[i] = 1;
		}
		// determine confidence values for each index
		for (i=0;i<(candidates->length)-1;i++){
			for (j=i+1;j<(candidates->length);j++){
				if ((candidates->array[j] -
				     candidates->array[i])<=xi) { 
					confidence[i]+=1;
					confidence[j]+=1;
				} else {
					break;
				}
			}
		}
		
		// determine which candidate has highest confidence
		// break ties by choosing the candidate with lower frequency
		maxConfidence = confidence[0];
		maxIndex = 0;
		for (i = 1; i<(candidates->length);i++){
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
			//		   (candidates->array[maxIndex]),
			//		   confidence[maxIndex]);
		}
		// determine first: index of first entry in candidates that is
		// within xi Hz of the candidate with the highest confidence
		first = maxIndex;
		for (i=maxIndex-1;i>=0;i--){
			if ((candidates->array[maxIndex] -
			     candidates->array[i])<=xi) {
				first = i;
			} else {
				break;
			}
		}

		// determine last: index of largest entry in candidates that is
		// within xi Hz of the candidate with the highest confidence
		last = maxIndex;
		for (i=maxIndex+1;i<(candidates->length);i++){
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
