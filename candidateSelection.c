#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "candidates.h"
#include "candidateSelection.h"



double costFunction(struct candidate cand1, struct candidate cand2)
{
	// calculate the cost for candidate 2 from candidate 1
	return abs(log(cand1.frequency/cand2.frequency)/log(2.0))+(0.4/(double)cand1.confidence);
}

double* candidateSelection(struct candidateList **windowLists, long length)
{
	// selects the candidates that represent the fundamentals

	double *fundamentals = malloc(sizeof(double)*length);

	long i=0;
	long start = 0;
	struct candidateList *curWindowList;
	struct candidateList *prevWindowList;

	while (i<length){
		// at this point in the loop, there are no candidates before
		// hand. By default all of the candidates in windowLists[start]
		// have a cost of zero and an indexLowestCost of zero
		// iterate through candidate lists until we encounter a list
		// with at least 1 element
    
		for (i=start;i<length;i++){
			curWindowList = windowLists[i];
			if (curWindowList->length!=0) {
				break;
			} else {
				fundamentals[i] = 0.0;
			}
		}

		if (i == length) {
			break;
		}
		start = i;
		// Iterate through the frames and calculate costs
		// stop when the last frame has been reached or when an empty
		// list is encountered
		for (i=start+1;i<length;i++){
			prevWindowList = curWindowList;
			curWindowList = windowLists[i];
			if (curWindowList->length==0) {
				break;
			} else {
				windowComparison(prevWindowList, curWindowList);
			}
		}

		// at this point we have either iterated through the candidate
		// lists for all windows OR we have hit an empty candidate list
		if (start-i==1) {

			// this is if window i has no candidates and it is
			// preceeded by only 1 window with candidates. In other
			// words, before the preceeding window there are no
			// windows or there is a window with zero candidates
			// alternatively, window i-1 was the last window and it
			// is either the first window or was preceeded by a
			// window with zero candidates

			// for both cases we need to do the same thing for
			// window i-1. for now we will just set the frequency
			// to the first candidate
			fundamentals[i-1] = ((prevWindowList->array[0]).frequency);

			// for the scenario where window i has no candidates,
			// the frequency will be set to 0 on the next loop
			// through 
		} else if (start-i > 1) {
			// this is if window i has 0 candidates or window i-1
			// was the last frame
			chooseLowestCost(fundamentals, windowLists,i-1,start);
		}
		start = i;
	}
	return fundamentals;
}

void windowComparison(struct candidateList *l1, struct candidateList *l2)
{
	// for each candidate in l2, calculate cost and indexLowestCost

	// before calling this function ensure that both lists have at least 1
	// candidate each

	struct candidate *cur;
	int i, j, indexLowestCost;
	double curCost, minCost;

	for (j=0;j<(l2->length); j++){
		cur = &(l2->array[j]);

		minCost = costFunction(l1->array[0],*cur);
		indexLowestCost = 0;

		for (i=1; i<(l1->length); i++){
			curCost = costFunction(l1->array[i],*cur);
			if (curCost<minCost) {
				minCost = curCost;
				indexLowestCost = i;
			}
		}
		candidateListAdjustCost(l2, j,minCost,indexLowestCost);
	}
}

void chooseLowestCost(double* fundamentals, struct candidateList **windowList,
		      long final, long start)
{
	// this function finds the lowest cost path from windowList[final] back
	// to windowList[start] and fills in fundamentals with the frequency
	// values first, find the lowest cost candidate in windowList[final]
	int index, indexLowestCost;
	double minCost, curCost;
	int numCandidates = windowList[final]->length;

	minCost = (windowList[final]->array[0]).cost;
	indexLowestCost = 0;
	for (index=1;index<numCandidates;index++){
		curCost = (windowList[final]->array[index]).cost;
		if (curCost<minCost) {
			minCost=curCost;
			indexLowestCost = index;
		}
	}

	// now trace the lowest cost path backwards
	long i;
	for (i=final;i>=start;i--){
		fundamentals[i] = (windowList[i]->array[indexLowestCost]).frequency;
		indexLowestCost = (windowList[i]->array[indexLowestCost]).indexLowestCost;
	}
}

/*
int main(int argc, char*argv[])
{
	return 0;
}
*/
