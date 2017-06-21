#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include "lists.h"
#include "candidateSelection.h"



float costFunction(struct distinctCandidate cand1,
		   struct distinctCandidate cand2)
{
	// calculate the cost for candidate 2 from candidate 1
	return (float)fabs(log(cand1.frequency/cand2.frequency)/log(2.0))+(0.4/(double)cand1.confidence);
}

float* candidateSelection(struct distinctList **windowList, long length)
{
	// selects the candidates that represent the fundamentals
	// finds blocks of candidates, and passes it to candidateSelectionSegment

	float *fundamentals = malloc(sizeof(float)*length);

	long i=0;
	long start = -1;
	long end = -1;
	int curWindowListLen;

	for ( i=0; i<length; i++){
		curWindowListLen = windowList[i]->length;

		if (curWindowListLen == 0){
			//not currently in a block of sound, set fundamental to 0
			fundamentals[i] = 0.0;
		}

		if (start == -1 && curWindowListLen != 0){
			//reached start of a block of sound. mark position
			start = i;
		}

		if (start != -1 && i == length-1){
			//reached end of block of sound. mark position
			end = i;
		}
		else if (start != -1 && curWindowListLen == 0){
			//passed end of block of sound. mark position
			end = i-1;
		}

		if(end != -1){
			//process block of sound
			if(start == end){
				//block was only 1 window long
				//for now we will just set the frequency to the first candidate
				fundamentals[start] = (float)((windowList[start]->array[0]).frequency);
			}
			else{
				candidateSelectionSegment(fundamentals, windowList,end,start);
			}
			start = -1;
			end = -1;
		}
	}
	return fundamentals;
}

void candidateSelectionSegment(float* fundamentals, 
			       struct distinctList **windowList, long final,
			       long start)
{
	// This function finds the lowest cost path from windowList[start]
	// to windowList[final] and fills in fundamentals with the frequency values.
	// Before calling this function ensure that all distinctLists from
	// windowList[start] to windowList[final] have at least 1 candidate each

	struct distinctList *curWindowList;
	struct distinctList *prevWindowList;
	struct distinctCandidate *curcandidate;
	long frame;
	int i, j, indexLowestCost;
	float curCost, minCost;

	//calculate intermediate mincost paths from start to final, eventually
	//finding lowest cost candidate in windowList[final]
	int finalindex = -1;
	float finalcost = FLT_MAX;
	for(frame = start + 1; frame <= final; ++frame){
		curWindowList = windowList[frame];
		prevWindowList = windowList[frame-1];

		for (i = curWindowList->length - 1; i >= 0; --i){
			curcandidate = &(curWindowList->array[i]);

			minCost = FLT_MAX;
			indexLowestCost = -1;

			for (j = prevWindowList->length - 1; j >= 0; --j){
				curCost = (prevWindowList->array[j]).cost;
				curCost += costFunction(prevWindowList->array[j],*curcandidate);
				if (curCost<=minCost) {
					//<= bc we favor candidates at lower freq if cost is equal
					minCost = curCost;
					indexLowestCost = j;
				}
			}
			distinctListAdjustCost(curWindowList, i,minCost,indexLowestCost);
			if(frame == final && minCost <= finalcost){
				finalcost = minCost;
				finalindex = i;
			}
		}
	}

	// now trace the lowest cost path backwards
	indexLowestCost = finalindex;
	for (frame = final; frame >= start; frame--){
		fundamentals[frame] = (float)(windowList[frame]->array[indexLowestCost]).frequency;
		indexLowestCost = (windowList[frame]->array[indexLowestCost]).indexLowestCost;
	}
}

/*
int main(int argc, char*argv[])
{
	return 0;
}
*/
