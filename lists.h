#ifndef LISTS_H
#define LISTS_H

struct orderedList{
	double *array;
	int max_length;
	int length;
};

struct unorderedList{
	int *array;
	int max_length;
	int length;
};

#endif /*LISTS_H*/

struct orderedList orderedListCreate(int max_length);
void orderedListDestroy(struct orderedList list);
double orderedListGet(struct orderedList list, int index);
int bisectInsertionSearch(struct orderedList list, double value);
void orderedListInsert(struct orderedList *list, double value);
void orderedListDeleteEntries(struct orderedList *list, int start, int stop);
void orderedListPrint(struct orderedList list);

struct unorderedList unorderedListCreate(int max_length);
void unorderedListDestroy(struct unorderedList list);
int unorderedListGet(struct unorderedList list, int index);
void unorderedListAppend(struct unorderedList *list, int value);
void unorderedListDeleteEntries(struct unorderedList *list, int start,
				int stop);
