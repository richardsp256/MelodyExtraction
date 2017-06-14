struct orderedList calcCandidates(double* peaks, int numPeaks);
float calcM(float f_i, float f_j);
struct distinctList* distinctCandidates(struct orderedList* candidates,
					int max_length, float xi,
					float f0Min, float f0Max);
