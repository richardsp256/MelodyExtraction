#ifndef LISTS_H
#define LISTS_H

#define DYNAMIC_ARR_MAX_CAPACITY 10000
#define DYNAMIC_ARR_GROWTH_FACTOR 2


// maintains an ordered list of floats. Don't actually need this to actively
// grow (we will know the correct size right off the bat)
//
// It would probably be worthwhile to refactor orderedList to use the same
// signatures as distinctList and intList
struct orderedList{
	float *array;
	int length;
	int capacity;
};

// Used by BaNa
struct distinctCandidate{
	float frequency;
	int confidence;
	float cost;
	int indexLowestCost;
};

typedef struct {
	struct distinctCandidate *array;
	int length;
	int capacity;
	int max_capacity;
} distinctList;

// more general purpose resizable list used to store results of Onset detection
// and silence detection
typedef struct {
	int *array;         // data
	int length;         // current length
	int capacity;       // current capacity
	int max_capacity;   // maximum possible capacity
} intList;

// used by the orderedList struct
int bisectLeft(float* l, float value, int low, int high);

struct orderedList orderedListCreate(int capacity);
void orderedListDestroy(struct orderedList list);
float orderedListGet(struct orderedList list, int index);
int bisectInsertionSearch(struct orderedList list, float value);
void orderedListInsert(struct orderedList *list, float value);
void orderedListDeleteEntries(struct orderedList *list, int start, int stop);
void orderedListPrint(struct orderedList list);

// If max_capacity is 0 or exceeds DYNAMIC_ARR_MAX_CAPACITY, then
// DYNAMIC_ARR_MAX_CAPACITY is used
distinctList* distinctListCreate(int capacity, int max_capacity);
void distinctListDestroy(distinctList* list);
struct distinctCandidate distinctListGet(distinctList *list, int index);
int distinctListAppend(distinctList *list, struct distinctCandidate);
void distinctListResize(distinctList *list);
void distinctListAdjustCost(distinctList *list, int index,
			     float cost, int indexLowestCost);
void distinctListPrintFreq(distinctList list);
void distinctListPrintConfidence(distinctList list);
void distinctListPrintCost(distinctList list);
void distinctListPrintIndexLowestCost(distinctList list);

// If max_capacity is 0 or exceeds DYNAMIC_ARR_MAX_CAPACITY, then
// DYNAMIC_ARR_MAX_CAPACITY is used
intList* intListCreate(int capacity, int max_capacity);
void intListDestroy(intList *list);
int intListGet(intList *list, int index);
void intListResize(intList *list);
int intListAppend(intList *list, int val);

#endif /*LISTS_H*/
