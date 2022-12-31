#include "../lists.h"

float costFunction(struct distinctCandidate cand1,
		   struct distinctCandidate cand2);
void candidateSelection(distinctList **windowList, long length,
			float* fundamentals);
void candidateSelectionSegment(float* fundamentals, distinctList **windowList,
			       long final, long start);
