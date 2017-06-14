struct orderedList calcCandidates(double* peaks, int numPeaks);
double calcM(float f_i, float f_j);
struct candidateList* distinctCandidates(struct orderedList* candidates,
					 int max_length, float xi,
					 float f0Min, float f0Max);
