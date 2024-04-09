#include <stdio.h>
#include <stdlib.h>
#include "lists.h"
#include "errors.h"

// note that these functions are not safe
// the user is responsible for making sure that the length of the list does
// not exceed the maximum size

/// find the index of the leftmost value in `l` greater than or equal to
/// `value`. It is specifically used by `orderedList`.
///
/// This algorithm was taken from cpython's biset_left function. An
/// implementation can be found here:
///     https://github.com/python-git/python/blob/master/Lib/bisect.py
///
/// Due to poor record keeping, it's not exactly clear which version of CPython
/// the bisectLeft function was derived from. Based on the commit history it
/// could have only been derived from versions 2.7 through 3.6 (it was probably
/// one of the extremes). In any case, the license is the same. A copy of the
/// license is provided in licenses/PYTHON_LICENSE
int bisectLeft(float* l, float value, int low, int high){

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

struct orderedList orderedListCreate(int capacity)
{
	struct orderedList list;
	list.array = malloc(capacity*sizeof(float));
	list.capacity = capacity;
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



/// @macro DYNAMIC_ARR_FUNCTIONS
/// Defines multiple methods of a dynamic array
/// @param ARR_NAME The type and name of the array
/// @param DTYPE The type of the contained values
///
/// Note: I got some weird errors when I tried to break the follow into smaller
///    pieces

#define DYNAMIC_ARR_FUNCTIONS(ARR_NAME, DTYPE)                              \
	ARR_NAME * ARR_NAME ## Create (int capacity)                        \
	{                                                                   \
		ARR_NAME *list;                                             \
		list = malloc(sizeof(ARR_NAME));                            \
		if (!list) {return NULL;}                                   \
		list->array = malloc(capacity * sizeof(DTYPE));             \
		if (!(list->array)){                                        \
			free(list);                                         \
			return NULL;                                        \
		}                                                           \
		list->capacity = capacity;                                  \
		list->length = 0;                                           \
		return list;                                                \
	}                                                                   \
	                                                                    \
	void ARR_NAME ## Destroy (ARR_NAME * list)                          \
	{                                                                   \
		free(list->array);                                          \
		free(list);                                                 \
	}                                                                   \
	                                                                    \
	DTYPE ARR_NAME ## Get (ARR_NAME * list, int index)                  \
	{                                                                   \
		return list->array[index];                                  \
	}                                                                   \
	                                                                    \
	int ARR_NAME ## Shrink (ARR_NAME * list)                            \
	{                                                                   \
		/* shrinks capacity to match length */                      \
		DTYPE * tmp;                                                \
		tmp = realloc(list->array, (list->length) * sizeof(DTYPE)); \
		if (tmp == NULL){                                           \
			return ME_REALLOC_FAILURE;                          \
		}                                                           \
		list->array = tmp;                                          \
		list->capacity = list->length;                              \
		return ME_SUCCESS;                                          \
	}                                                                   \
	                                                                    \
	int ARR_NAME ## Append (ARR_NAME * list, DTYPE val)                 \
	{                                                                   \
		if (list->length == list->capacity){                        \
			DTYPE * tmp;                                        \
			list->capacity *= DYNAMIC_ARR_GROWTH_FACTOR;        \
			tmp = realloc(list->array,                          \
				     list->capacity * sizeof(DTYPE));       \
			if (!tmp){                                          \
				return ME_REALLOC_FAILURE;                  \
			}                                                   \
			list->array = tmp;                                  \
		}                                                           \
		(list->array)[list->length] = val;                          \
		(list->length)++;                                           \
		return ME_SUCCESS;                                          \
	}

	
DYNAMIC_ARR_FUNCTIONS(distinctList, struct distinctCandidate);
// implements:
//    distinctList* distinctListCreate(int capacity)
//    void distinctListDestroy(distinctList *list)
//    struct distinctCandidate distinctListGet(distinctList *list, int index)
//    int distinctListShrink(distinctList *list)
//    int distinctListAppend(distinctList *list, struct distinctCandidate val)


// The following provides some basic utility
void distinctListAdjustCost(distinctList *list, int index,
			    float cost, int indexLowestCost)
{
	// adjust values of cost and indexLowestCost for list[index]
	struct distinctCandidate* tmp;
	tmp = &(list->array[index]);
	tmp->cost = cost;
	tmp->indexLowestCost = indexLowestCost;
}

// The following were written to assist with debugging
void distinctListPrintFreq(distinctList list)
{
	printf("[");
	if (list.length != 0) {
		printf("%lf",list.array[0].frequency);
	}
	for (int i=1; i<list.length; i++){
		printf(", %lf",list.array[i].frequency);
	}
	printf("]\n");
}

void distinctListPrintConfidence(distinctList list)
{
	printf("[");
	if (list.length != 0){
		printf("%d",list.array[0].confidence);
	}
	for (int i=1; i<list.length; i++){
		printf(", %d",list.array[i].confidence);
	}
	printf("]\n");
}

void distinctListPrintCost(distinctList list)
{
	printf("[");
	if (list.length != 0){
		printf("%.10f",list.array[0].cost);
	}
	for (int i=1; i<list.length; i++){
		printf(", %.10f",list.array[i].cost);
	}
	printf("]\n");
}

void distinctListPrintIndexLowestCost(distinctList list)
{
	printf("[");
	if (list.length != 0){
		printf("%d",list.array[0].indexLowestCost);
	}
	for (int i=1; i<list.length; i++){
		printf(", %d",list.array[i].indexLowestCost);
	}
	printf("]\n");
}

DYNAMIC_ARR_FUNCTIONS(intList, int);
// implements:
//    intList* intListCreate(int capacity)
//    void intListDestroy(intList *list)
//    int intListGet(intList *list, int index)
//    int intListShrink(intList *list)
//    int intListAppend(intList *list, int val)
