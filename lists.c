#include <stdio.h>
#include <stdlib.h>
#include "lists.h"

// note that these functions are not safe
// the user is responsible for making sure that the length of the list does
// not exceed the maximum size

// Here, I define an ordered list of doubles, orderedList, and an unordered list
// of ints unorderedList

// Question: Why do I need to pass the pointer to the struct to modify
// non-pointer members? What happens when I try to modify the non-pointer
// members without passing the struct as a pointer?

// can get rid of unorderedList

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

	// We just use the algorithm used for python's bisect left function
	// It is implemented here:
	// https://github.com/python-git/python/blob/master/Lib/bisect.py

	// This could probably be better optimized

	// for integers >=0 integer division is floor division
	int low = 0;
	int high = list.length;
	int mid;
	while (low<high){
		mid = (low+high)/2;
		if (list.array[mid]<value){
			low = mid+1;
		} else {
			high = mid;
		}
	}
	return low;
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


// Defining functions for unorderedList

struct unorderedList unorderedListCreate(int max_length)
{
	struct unorderedList list;
	list.array = malloc(max_length*sizeof(int));
	list.max_length = max_length;
	list.length = 0;
	return list;	
}

void unorderedListDestroy(struct unorderedList list)
{
	free(list.array);
}

int unorderedListGet(struct unorderedList list, int index)
{
	return list.array[index];
}

void unorderedListAppend(struct unorderedList *list, int value)
{
	(list->array)[list->length] = value;
	(list->length)++;
}

void unorderedListDeleteEntries(struct unorderedList *list, int start,
				int stop)
{
	// exact same code as orderedListDeleteEntries
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

void unorderedListPrint(struct unorderedList list)
{
	printf("[");
	if (list.length != 0){
		printf("%d",list.array[0]);
	}
	int i;
	for (i=1; i<list.length; i++){
		printf(", %d",list.array[i]);
	}
	printf("]\n");
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

	struct unorderedList ul = unorderedListCreate(18);
	unorderedListPrint(ul);
	unorderedListAppend(&ul, 63);
	unorderedListPrint(ul);
	unorderedListAppend(&ul, 1244);
	unorderedListPrint(ul);
	unorderedListAppend(&ul, 542);
	unorderedListPrint(ul);
	unorderedListAppend(&ul, 17);
	unorderedListPrint(ul);
	unorderedListDeleteEntries(&ul, 1, 3);
	unorderedListPrint(ul);
	unorderedListAppend(&ul, 89);
	unorderedListPrint(ul);
	unorderedListDeleteEntries(&ul, 0, 2);
	unorderedListPrint(ul);    
	unorderedListDestroy(ul);
	return 0;
}
*/
