#include <stdlib.h>
#include <stdio.h>
#include "lists.h"
#include "findCandidates.h"

// https://stackoverflow.com/questions/6514651/declare-large-global-array
static double ratioRanges[15] = {1.15, 1.29, 1.42, 1.59,  
				  1.8,  1.9,  2.1,  2.4,
				  2.6,  2.8,  3.2,  3.8,
				  4.2,  4.8,  5.2};

static double mRanges[15] = { 4,  3,  2,  3,  
			     -1,  1, -1,  2,
			     -1,  1, -1,  1,  
			     -1,  1, -1};


struct orderedList calcCandidates(double* peaks, int numPeaks)
{
	int i,j;
	double m;
	struct orderedList candidates = orderedListCreate(2+((numPeaks-1) *
							     (numPeaks-1)));

	for (i=0; i<numPeaks-1;i++){
		for (j=i+1; j<numPeaks; j++){
			m = calcM(peaks[i],peaks[j]);
			if (m>0) {
				orderedListInsert(&candidates,peaks[i]/m);
			}				
		}
	}
	return candidates;
}
 
double calcM(double f_i, double f_j){
	// find the index, i, of the right most value in ratioRanges less than
        // f_j/f_i. it returns mRanges[i]
	double ratio = f_j/f_i;

	// basically using algorithm from python's bisect left and then moving
	// one index to the left. This could probably be improved

	int low = 0;
	int high = 15;
	int mid;

	while (low<high){
		mid = (low+high)/2;
		if (ratioRanges[mid]<ratio) {
			low = mid+1;
		} else {
			high = mid;
		}
	}

	if (low == 0) {
		return -1.;
	}

	int i= low - 1;

	return mRanges[i];
}


struct candidateList* distinctCandidates(struct orderedList* candidates,
					 int max_length, double xi,
					 double f0Min, double f0Max)
{	
	// Finds the distinct candidates from the list of candidates. This is
        // done by selecting the candidate with the most other candidates
        // within xi Hz
	
	int first, last, i,j,maxIndex;
	double* confidence, maxConfidence;
	struct candidateList *distinct;
	distinct = (candidateListCreate(max_length));

	confidence = malloc(sizeof(double) * (candidates->length));

	first = -1;

	while (candidates->length !=0){	        
		// set entries of confidence to 1
		for (i=0;i<(candidates->length);i++){
			confidence[i] = 1.0;
		}
		// determine confidence values for each index
		for (i=0;i<(candidates->length)-1;i++){
			for (j=i+1;j<(candidates->length);j++){
				if ((candidates->array[j] -
				     candidates->array[i])<=xi) { 
					confidence[i]+=1.0;
					confidence[j]+=1.0;
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
			candidateListAdd(distinct, candidates->array[maxIndex],
					 confidence[maxIndex]);
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
