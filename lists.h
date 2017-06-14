#ifndef LISTS_H
#define LISTS_H

struct orderedList{
	float *array;
	int max_length;
	int length;
};

struct distinctCandidate{
  float frequency;
  int confidence;
  float cost;
  int indexLowestCost;
};

struct distinctList{
  struct distinctCandidate *array;
  int max_length;
  int length;
};

#endif /*LISTS_H*/

int bisectLeftD(double* l, double value, int low, int high);
int bisectLeft(float* l, float value, int low, int high);
struct orderedList orderedListCreate(int max_length);
void orderedListDestroy(struct orderedList list);
float orderedListGet(struct orderedList list, int index);
int bisectInsertionSearch(struct orderedList list, float value);
void orderedListInsert(struct orderedList *list, float value);
void orderedListDeleteEntries(struct orderedList *list, int start, int stop);
void orderedListPrint(struct orderedList list);

struct distinctList* distinctListCreate(int max_length);
void distinctListDestroy(struct distinctList* list);
struct distinctCandidate distinctListGet(struct distinctList list, int index);
void distinctListAdd(struct distinctList *list, float frequency,
		      int confidence);
void distinctListResize(struct distinctList *list);
void distinctListAdjustCost(struct distinctList *list, int index,
			     float cost, int indexLowestCost);
void distinctListPrintFreq(struct distinctList list);
void distinctListPrintConfidence(struct distinctList list);
