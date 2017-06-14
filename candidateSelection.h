#include "lists.h"

float costFunction(struct distinctCandidate cand1,
		   struct distinctCandidate cand2);
float* candidateSelection(struct distinctList **windowList, long length);
void candidateSelectionSegment(float* fundamentals,
			       struct distinctList **windowList, long final,
			       long start);
