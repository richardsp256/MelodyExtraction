#include <stdio.h>
#include <stdlib.h>
#include "lists.h"

// note that these functions are not safe
// the user is responsible for making sure that the length of the list does
// not exceed the maximum size

// below is afunction generally associated with lists. It is specifically used
// by orderedList

int bisectLeft(float* l, float value, int low, int high){
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

int bisectLeftD(double* l, double value, int low, int high){
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
	list.array = malloc(max_length*sizeof(float));
	list.max_length = max_length;
	list.length = 0;
	return list;
}

void orderedListDestroy(struct orderedList list)
{
	free(list.array);
}

float orderedListGet(struct orderedList list, int index)
{
	return list.array[index];
}

int bisectInsertionSearch(struct orderedList list, float value){
	// function to find the index of the leftmost value in list greater
	// than or equal to value.

	return bisectLeft(list.array, value, 0, list.length);
}

void orderedListInsert(struct orderedList *list, float value)
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
		printf("%f",list.array[0]);
	}
	int i;
	for (i=1; i<list.length; i++){
		printf(", %f",list.array[i]);
	}
	printf("]\n");
}

// Defining functions for distinctList (which is an unordered list containing
// candidate structs

struct distinctList* distinctListCreate(int max_length)
{
	struct distinctList* list;
	list = malloc(sizeof(struct distinctList));
	list->array = malloc(max_length * sizeof(struct distinctCandidate));
	list->max_length = max_length;
	list->length = 0;
	return list;
}

void distinctListDestroy(struct distinctList* list)
{
	free(list->array);
	free(list);
}

struct distinctCandidate distinctListGet(struct distinctList list, int index)
{
	return list.array[index];
}

void distinctListAdd(struct distinctList *list, float frequency,
		      int confidence)
{
	struct distinctCandidate temp;
	temp.frequency = frequency;
	temp.confidence = confidence;
	temp.cost = 0.0;
	temp.indexLowestCost = -1;
	(list->array)[list->length] = temp;
	(list->length)++;
}

void distinctListPrintFreq(struct distinctList list)
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

void distinctListPrintConfidence(struct distinctList list)
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

void distinctListResize(struct distinctList *list)
{
	// resizes the list to be the current size of the candidate list
	struct distinctCandidate* tmp;
	tmp = realloc(list->array, (list->length) *
		      sizeof(struct distinctCandidate));
	if (tmp == NULL){
		// this means there was a failure here, we should do something
	}
	list->array = tmp;
	list->max_length = list->length;
}

void distinctListAdjustCost(struct distinctList *list, int index,
			     float cost, int indexLowestCost)
{
	// adjust values of cost and indexLowestCost for list[index]
	struct distinctCandidate* tmp;
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

	struct distinctList* lptr = distinctListCreate(18);
	struct distinctList l = *lptr;
	distinctListAdd(&l, 190.0,7);
	distinctListAdd(&l, 96.,2);
	distinctListAdd(&l, 121,1);
	distinctListAdd(&l, 242,1);
	distinctListAdd(&l, 391,1);

	distinctListPrintFreq(l);
	distinctListPrintConfidence(l);
	printf("%d\n",(l.max_length));
	distinctListResize(&l);
	distinctListPrintFreq(l);
	distinctListPrintConfidence(l);
	printf("%d\n",(l.max_length));

	printf("cost = %lf  indexLowestCost = %d\n", l.array[2].cost,
	       l.array[2].indexLowestCost);

	distinctListAdjustCost(&l, 2, 56.4, 1);
	printf("cost = %lf  indexLowestCost = %d\n", l.array[2].cost,
	       l.array[2].indexLowestCost);


	struct distinctList *l2ptr = distinctListCreate(18);
	struct distinctList l2 = *l2ptr;
	distinctListAdd(&l2, 532.24,2);
	distinctListAdd(&l2, 68.1,1);
	distinctListResize(&l2);

	struct distinctList **windowLists = malloc(sizeof(struct distinctList*)*2);
	windowLists[0] = &l;
	windowLists[1] = &l2;

	distinctListPrintFreq(*windowLists[1]);
	distinctListPrintFreq(*windowLists[0]);
	free(windowLists);
	distinctListDestroy(lptr);
	distinctListDestroy(l2ptr);

}
*/
