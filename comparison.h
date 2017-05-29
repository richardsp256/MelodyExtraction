#include "sndfile.h"
#include "fftw3.h"
#include "detectionStrat.h"

void PrintAudioMetadata(SF_INFO * file);
float* WindowFunction(int size);
int ExtractMelody(char* inFile, char* outFile, int winSize, int winInt, int hpsOvr, int verbose, char* prefix, PitchDetectionStrategyFunc detectionStrategy);
int STFT(double** input, SF_INFO info, int winSize, int interval, fftw_complex** dft_data);
int STFTinverse(fftw_complex** input, SF_INFO info, int winSize, int interval, double** output);
double* Magnitude(fftw_complex* arr, int size);
void SaveAsWav(const double* audio, SF_INFO info, const char* path);
void SaveWeightsTxt(char* fileName, double** AudioData, int size, int dftBlocksize, int samplerate, int winSize);
