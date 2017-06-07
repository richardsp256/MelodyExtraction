#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "lists.h"
#include "candidateSelection.h"



double costFunction(struct candidate cand1, struct candidate cand2)
{
	// calculate the cost for candidate 2 from candidate 1
	return abs(log(cand1.frequency/cand2.frequency)/log(2.0))+(0.4/(double)cand1.confidence);
}

double* candidateSelection(struct candidateList **windowList, long length)
{
	// selects the candidates that represent the fundamentals

	double *fundamentals = malloc(sizeof(double)*length);

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
				fundamentals[start] = ((windowList[start]->array[0]).frequency);
			}
			else{
				chooseLowestCost(fundamentals, windowList,end,start);
			}
			start = -1;
			end = -1;
		}
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
	struct candidateList *curWindowList;
	struct candidateList *prevWindowList;
	for(int i = start + 1; i <= final; i++){
		curWindowList = windowList[i];
		prevWindowList = windowList[i-1];
		windowComparison(prevWindowList, curWindowList);
	}
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
