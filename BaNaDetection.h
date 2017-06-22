#include "lists.h"
float* BaNa(float **AudioData, int size, int dftBlocksize, int p,
	    float f0Min, float f0Max, float xi, int fftSize, int samplerate,
	    int first);
float* calcFrequencies(int dftBlocksize, int fftSize, int samplerate);
void BaNaPreprocessing(float **AudioData, int size, int dftBlocksize, int p,
		       float f0Min, float f0Max, float* frequencies);
struct distinctList** BaNaFindCandidates(float **AudioData, int size,
					 int dftBlocksize, int p, float f0Min,
					 float f0Max, int first, float xi,
					 float* frequencies, int fftSize,
					 int samplerate);
