#include "candidates.h"

double costFunction(struct candidate cand1, struct candidate cand2);
double* candidateSelection(struct candidateList **windowLists, long length);
void windowComparison(struct candidateList *l1, struct candidateList *l2);
void chooseLowestCost(double* fundamentals, struct candidateList **windowList,
		      long final, long start);
