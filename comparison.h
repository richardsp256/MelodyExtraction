#include "sndfile.h"
#include "fftw3.h"

void PrintAudioMetadata(SF_INFO * file);
float* WindowFunction(int size);
int ExtractMelody(char* inFile, char* outFile, int winSize, int winInt, int hpsOvr, int verbose);
int STFT(double** input, SF_INFO info, int winSize, int interval, fftw_complex** dft_data);
int STFTinverse(fftw_complex** input, SF_INFO info, int winSize, int interval, double** output);
void HarmonicProductSpectrum(fftw_complex** AudioData, int size, int dftBlocksize, int hpsOvr);
void SaveAsWav(const double* audio, SF_INFO info, const char* path);