#include "../lists.h"
struct orderedList calcCandidates(const float* peaks, int numPeaks);
float calcM(float f_i, float f_j);
distinctList* distinctCandidates(struct orderedList* candidates,
				 int max_length, float xi,
				 float f0Min, float f0Max);
