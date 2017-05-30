#include "lists.h"
float* BaNa(double **AudioData, int size, int dftBlocksize, int p,
	     double f0Min, double f0Max, int fftSize, int samplerate);
float* BaNaMusic(double **AudioData, int size, int dftBlocksize, int p,
		  double f0Min, double f0Max, int fftSize, int samplerate);
double* calcFrequencies(int dftBlocksize, int fftSize, int samplerate);
void BaNaPreprocessing(double **AudioData, int size, int dftBlocksize, int p,
		       double f0Min, double f0Max, double* frequencies);
struct candidateList** BaNaFindCandidates(double **AudioData, int size,
					int dftBlocksize, int p, double f0Min,
					double f0Max, int first, double xi,
					double* frequencies, int fftSize,
					int samplerate);
