#include "sndfile.h"
#include "fftw3.h"
#include "pitchStrat.h"
#include "onsetStrat.h"

void PrintAudioMetadata(SF_INFO * file);
float* WindowFunction(int size);
int ExtractMelody(char* inFile, char* outFile, 
		int p_winSize, int p_winInt, PitchStrategyFunc pitchStrategy,
		int o_winSize, int o_winInt, OnsetStrategyFunc onsetStrategy,
		int hpsOvr, int verbose, char* prefix);
int STFT(double** input, SF_INFO info, int winSize, int interval, fftw_complex** dft_data);
int STFTinverse(fftw_complex** input, SF_INFO info, int winSize, int interval, double** output);
double* Magnitude(fftw_complex* arr, int size);
void SaveAsWav(const double* audio, SF_INFO info, const char* path);
void SaveWeightsTxt(char* fileName, double** AudioData, int size, int dftBlocksize, int samplerate, int winSize);
