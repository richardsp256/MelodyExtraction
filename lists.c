#include <stdio.h>
#include <stdlib.h>
#include "lists.h"

// note that these functions are not safe
// the user is responsible for making sure that the length of the list does
// not exceed the maximum size

// below is afunction generally associated with lists. It is specifically used
// by orderedList

int bisectLeft(double* l, double value, int low, int high){
	// function to find the index of the leftmost value in l greater
	// than or equal to value.

	// We just use the algorithm used for python's bisect left function
	// It is implemented here:
	// https://github.com/python-git/python/blob/master/Lib/bisect.py

	// This could probably be better optimized

	// for integers >=0 integer division is floor division
	int mid;
	while (low<high){
		mid = (low+high)/2;
		if (l[mid]<value){
			low = mid+1;
		} else {
			high = mid;
		}
	}
	return low;
}



// Defining functions for orderedList

struct orderedList orderedListCreate(int max_length)
{
	struct orderedList list;
	list.array = malloc(max_length*sizeof(double));
	list.max_length = max_length;
	list.length = 0;
	return list;
}

void orderedListDestroy(struct orderedList list)
{
	free(list.array);
}

double orderedListGet(struct orderedList list, int index)
{
	return list.array[index];
}

int bisectInsertionSearch(struct orderedList list, double value){
	// function to find the index of the leftmost value in list greater
	// than or equal to value.

	return bisectLeft(list.array, value, 0, list.length);
}

void orderedListInsert(struct orderedList *list, double value)
{
	// insert value into list
	if (list->length == 0){
		(list->array)[0] = value;
	} else {
		// Calculate index where we will insert the value
		int index = bisectInsertionSearch(*list, value);
		// shift all of the entries with indices >= index to the right
		// by 1
		int i;
		for (i = (list->length)-1;i>=index;i--){
			(list->array)[i+1] = (list->array)[i];
		}
		// set the array at index equal to the value
		(list->array)[index] = value;
	}
	(list->length)++;
}

void orderedListDeleteEntries(struct orderedList *list, int start, int stop)
{
	// this function deletes all entries in list with indices
	// start,...,stop-1

	if (start>=stop){
		return;
	}

	int n = stop - start;
	int i;
	for (i = stop; i<(list->length);i++){
		(list->array)[i-n] = (list->array)[i];
	}
	(list->length) -= n;
}


void orderedListPrint(struct orderedList list)
{
	printf("[");
	if (list.length != 0){
		printf("%lf",list.array[0]);
	}
	int i;
	for (i=1; i<list.length; i++){
		printf(", %lf",list.array[i]);
	}
	printf("]\n");
}

// Defining functions for candidateList (which is an unordered list containing
// candidate structs

struct candidateList* candidateListCreate(int max_length)
{
	struct candidateList* list;
	list = malloc(sizeof(struct candidateList));
	list->array = malloc(max_length * sizeof(struct candidate));
	list->max_length = max_length;
	list->length = 0;
	return list;
}

void candidateListDestroy(struct candidateList* list)
{
	free(list->array);
	free(list);
}

struct candidate candidateListGet(struct candidateList list, int index)
{
	return list.array[index];
}

void candidateListAdd(struct candidateList *list, double frequency,
		      int confidence)
{
	struct candidate temp;
	temp.frequency = frequency;
	temp.confidence = confidence;
	temp.cost = 0.0;
	temp.indexLowestCost = -1;
	(list->array)[list->length] = temp;
	(list->length)++;
}

void candidateListPrintFreq(struct candidateList list)
{
	printf("[");
	if (list.length != 0) {
		printf("%lf",list.array[0].frequency);
	}
	int i;
	for (i=1; i<list.length; i++){
		printf(", %lf",list.array[i].frequency);
	}
	printf("]\n");
}

void candidateListPrintConfidence(struct candidateList list)
{
	printf("[");
	if (list.length != 0){
		printf("%d",list.array[0].confidence);
	}
	int i;
	for (i=1; i<list.length; i++){
		printf(", %d",list.array[i].confidence);
	}
	printf("]\n");
}

void candidateListResize(struct candidateList *list)
{
	// resizes the list to be the current size of the candidate list
	struct candidate* tmp;
	tmp = realloc(list->array,(list->length)*sizeof(struct candidate));
	if (tmp == NULL){
		// this means there was a failure here, we should do something
	}
	list->array = tmp;
	list->max_length = list->length;
}

void candidateListAdjustCost(struct candidateList *list, int index,
			     double cost, int indexLowestCost)
{
	// adjust values of cost and indexLowestCost for list[index]
	struct candidate* tmp;
	tmp = &(list->array[index]);
	tmp->cost = cost;
	tmp->indexLowestCost = indexLowestCost;
}

/*
int main(int argc, char*argv[])
{
	// only including this for testing purposes
	struct orderedList l = orderedListCreate(18);
	orderedListPrint(l);
	orderedListInsert(&l, 63.4);
	orderedListPrint(l);
	orderedListInsert(&l, 1244.);
	orderedListPrint(l);
	orderedListInsert(&l, 542.3);
	orderedListPrint(l);
	orderedListInsert(&l, 17.8);
	orderedListPrint(l);
	orderedListDeleteEntries(&l, 1, 3);
	orderedListPrint(l);
	orderedListInsert(&l, 89.43);
	orderedListPrint(l);
	orderedListInsert(&l, 17.8);
	orderedListInsert(&l, 89.43);
	orderedListPrint(l);
	orderedListDeleteEntries(&l, 0, 2);
	orderedListPrint(l);
	orderedListDeleteEntries(&l, 0, 3);
	orderedListPrint(l);
	orderedListDestroy(l);

	struct candidateList* lptr = candidateListCreate(18);
	struct candidateList l = *lptr;
	candidateListAdd(&l, 190.0,7);
	candidateListAdd(&l, 96.,2);
	candidateListAdd(&l, 121,1);
	candidateListAdd(&l, 242,1);
	candidateListAdd(&l, 391,1);

	candidateListPrintFreq(l);
	candidateListPrintConfidence(l);
	printf("%d\n",(l.max_length));
	candidateListResize(&l);
	candidateListPrintFreq(l);
	candidateListPrintConfidence(l);
	printf("%d\n",(l.max_length));

	printf("cost = %lf  indexLowestCost = %d\n", l.array[2].cost,
	       l.array[2].indexLowestCost);

	candidateListAdjustCost(&l, 2, 56.4, 1);
	printf("cost = %lf  indexLowestCost = %d\n", l.array[2].cost,
	       l.array[2].indexLowestCost);


	struct candidateList *l2ptr = candidateListCreate(18);
	struct candidateList l2 = *l2ptr;
	candidateListAdd(&l2, 532.24,2);
	candidateListAdd(&l2, 68.1,1);
	candidateListResize(&l2);

	struct candidateList **windowLists = malloc(sizeof(struct candidateList*)*2);
	windowLists[0] = &l;
	windowLists[1] = &l2;

	candidateListPrintFreq(*windowLists[1]);
	candidateListPrintFreq(*windowLists[0]);
	free(windowLists);
	candidateListDestroy(lptr);
	candidateListDestroy(l2ptr);

}
*/
