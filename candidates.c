#include <stdlib.h>
#include <stdio.h>
#include "candidates.h"

// When I allocate an array of structs, does c automatically make empty structs?

// Here I define candidate and an unordered list to hold candidates


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
