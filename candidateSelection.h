#include "lists.h"

float costFunction(struct candidate cand1, struct candidate cand2);
float* candidateSelection(struct candidateList **windowList, long length);
void candidateSelectionSegment(float* fundamentals, 
				struct candidateList **windowList, long final, long start);
