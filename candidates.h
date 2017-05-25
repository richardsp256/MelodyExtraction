#ifndef CANDIDATES_H
#define CANDIDATES_H

struct candidate{
  double frequency;
  int confidence;
  double cost;
  int indexLowestCost;
};

struct candidateList{
  struct candidate *array;
  int max_length;
  int length;
};
#endif /*CANDIDATES_H*/


struct candidateList* candidateListCreate(int max_length);
void candidateListDestroy(struct candidateList* list);
struct candidate candidateListGet(struct candidateList list, int index);
void candidateListAdd(struct candidateList *list, double frequency,
		      int confidence);
void candidateListResize(struct candidateList *list);
void candidateListAdjustCost(struct candidateList *list, int index,
			     double cost, int indexLowestCost);
void candidateListPrintFreq(struct candidateList list);
void candidateListPrintConfidence(struct candidateList list);
