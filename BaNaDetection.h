#include "candidates.h"
double* BaNa(double **AudioData, int size, int dftBlocksize, int p,
	     double f0Min, double f0Max, int fftSize, int samplerate);
double* BaNaMusic(double **AudioData, int size, int dftBlocksize, int p,
		  double f0Min, double f0Max, int fftSize, int samplerate);
double* calcFrequencies(int dftBlocksize, int fftSize, int samplerate);
void BaNaPreprocessing(double **AudioData, int size, int dftBlocksize, int p,
		       double f0Min, double f0Max, double* frequencies);
void BaNaFindCandidates(double **AudioData,int size,int dftBlocksize, int p,
			double f0Min, double f0Max, int first,
			struct candidateList **windowCandidates,
			double xi, double* frequencies, int fftSize,
			int samplerate);
int* FreqToBin(double* fundamentals, int fftSize, int samplerate, int size,
	       int dftBlocksize);
