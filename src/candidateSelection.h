#include "lists.h"

float costFunction(struct distinctCandidate cand1,
		   struct distinctCandidate cand2);
float* candidateSelection(distinctList **windowList, long length);
void candidateSelectionSegment(float* fundamentals, distinctList **windowList,
			       long final, long start);
