struct orderedList calcCandidates(double* peaks, int numPeaks);
double calcM(double f_i, double f_j);
struct candidateList* distinctCandidates(struct orderedList* candidates,
					 int max_length, double xi);
