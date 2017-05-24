#include <stdlib.h>
#include <stdio.h>
#include "candidates.h"
#include "lists.h"
#include "findCandidates.h"

// https://stackoverflow.com/questions/6514651/declare-large-global-array
double ratioRanges[15] = {1.15, 1.29, 1.42, 1.59,  
                           1.8,  1.9,  2.1,  2.4,
                           2.6,  2.8,  3.2,  3.8,
       			   4.2,  4.8,  5.2};

double mRanges[15] = { 4,  3,  2,  3,  
		      -1,  1, -1,  2,
                      -1,  1, -1,  1,  
		      -1,  1, -1};




struct orderedList calcCandidates(double* peaks, int numPeaks){
	int i,j;
	double m;
	struct orderedList candidates = orderedListCreate(2+((numPeaks-1) *
							     (numPeaks-1)));
	
	for (i=0; i<numPeaks-1;i++){
		for (j=i+1; j<numPeaks; j++){
			m = calcM(peaks[i],peaks[j]);
			if (m>0){
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
	  if (ratioRanges[mid]<ratio){
	    low = mid+1;
	  }
	  else{
	    high = mid;
	  }
	}

	if (low == 0){
	  return -1.;
	}
	
	int i= low - 1;
	
	return mRanges[i];
}


struct candidateList* distinctCandidates(struct orderedList* candidates,
					 int max_length, double xi){
	
	// Finds the distinct candidates from the list of candidates. This is
        // done by selecting the candidate with the most other candidates
        // within xi Hz
	
	int first, last, i,j,maxIndex;
	double* confidence, maxConfidence;
	struct candidateList *distinct;
	distinct = (candidateListCreate(max_length));
	
	confidence = malloc(sizeof(double) * (candidates->length));
	
	first = -1;

	//printf("\n");
	while (candidates->length !=0){
	        /*
	        printf("Candidates:\n");
	        orderedListPrint(*candidates);
	        printf("length %d\n", candidates->length);
                */
		// set entries of confidence to 1
		for (i=0;i<(candidates->length);i++){
		       confidence[i] = 1.0;
		}
		// determine confidence values for each index
		for (i=0;i<(candidates->length)-1;i++){
			for (j=i+1;j<(candidates->length);j++){
				if ((candidates->array[j]-candidates->array[i])<=xi){ 
					confidence[i]+=1.0;
					confidence[j]+=1.0;
				}
				else{
					break;
				}
			}
		}
		/*
		printf("Confidence:\n");
		printf("[%f",confidence[0]);
		for (i = 1;i<candidates->length;i++){
		  printf(", %f", confidence[i]);
		}
		printf("]\n");
		*/
		
		// determine which candidate has highest confidence
		// break ties by choosing the candidate with lower frequency
		maxConfidence = confidence[0];
		maxIndex = 0;
		for (i = 1; i<(candidates->length);i++){
			if (confidence[i]>maxConfidence){
				maxIndex = i;
				maxConfidence = confidence[i];
			}
		}
		/*
		printf("maxIndex = %d\n", maxIndex);
		printf("distinctive frequency: %f\n",
		       candidates->array[maxIndex]);
		printf("Number of close candidates: %d\n", (int)maxConfidence);
		*/
		// add that candidate to distinct
		candidateListAdd(distinct, candidates->array[maxIndex],
				 confidence[maxIndex]);
		
		// determine first: index of first entry in candidates that is within 
		// xi Hz of the candidate with the highest confidence
		first = maxIndex;
		for (i=maxIndex-1;i>=0;i--){
			if ((candidates->array[maxIndex] - candidates->array[i])<=xi){
				first = i;
			}
			else{
				break;
			}
		}

		// determine last: index of largest entry in candidates that is within
		// xi Hz of the candidate with the highest confidence
		last = maxIndex;
		for (i=maxIndex+1;i<(candidates->length);i++){
			if ((candidates->array[i] - candidates->array[maxIndex])<=xi){
				last = i;
			}
			else{
				break;
			}
		}
		
		// remove that candidate and all of the candidates within xi Hz 
		// from that candidate from candidates
		/*
		printf("first: %d\n", first);
		printf("last: %d\n", last+1);
		*/
		orderedListDeleteEntries(candidates, first, last+1);
		
		//printf("\n");
		
	}
	
	free(confidence);
	return distinct;
}


/*
int main(int argc, char*argv[]){

  // example from paper
  int length;
  struct candidateList *distinct;
  double *peaks = malloc(5*sizeof(double));
  peaks[0] = 192;
  peaks[1] = 391;
  peaks[2] = 485;
  peaks[3] = 581;
  peaks[4] = 760;

  length = 5;
  
  struct orderedList candidates = calcCandidates(peaks, length);

  // add the lowest frequency peak
  orderedListInsert(&candidates,192.);
  // add the cepstrum fundamental
  orderedListInsert(&candidates,190.);
  printf("candidates:");
  orderedListPrint(candidates);
  
  distinct = distinctCandidates(&candidates,18, 10);
  
  printf("\nDistinct Candidates:\n");
  candidateListPrintFreq(*distinct);
  printf("Confidence Scores:\n");
  candidateListPrintConfidence(*distinct);  
  
  orderedListDestroy(candidates);
  candidateListDestroy(distinct);
  free(peaks);
  
  return 0;
}
*/
