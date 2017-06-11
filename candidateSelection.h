#include "lists.h"

double costFunction(struct candidate cand1, struct candidate cand2);
double* candidateSelection(struct candidateList **windowList, long length);
void candidateSelectionSegment(double* fundamentals, 
				struct candidateList **windowList, long final, long start);