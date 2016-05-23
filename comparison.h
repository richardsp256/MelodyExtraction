#include "sndfile.h"
#include "fftw3.h"

void PrintAudioMetadata(SF_INFO * file);
int ReadAudioFile(char* filename, double*** dft_data, unsigned int* samplerate, unsigned int* frames);
int PassAudioData(double* samples, int numSamples, double*** dft_data, double** fftw_in, fftw_complex** fftw_out, fftw_plan* fftw_plan);
double GetFitnessHelper(double** goal, double** test, int size);
double AudioComparison(double* samples, int numSamples, double** goal, int goalsize, double** fftw_in, fftw_complex** fftw_out, fftw_plan* fftw_plan);