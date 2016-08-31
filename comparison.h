#include "sndfile.h"
#include "fftw3.h"

void PrintAudioMetadata(SF_INFO * file);
float* WindowFunction(int size);
int ExtractMelody(char* inFile, char* outFile);
int STFT(double** signal, SF_INFO info, int blocksize, int interval, double*** dft_data);
int STFTinverse(double*** input, SF_INFO info, int blocksize, int interval, double** output);
void HarmonicProductSpectrum(double*** AudioData, int size, int dftBlocksize);
void SaveAsWav(const double* audio, SF_INFO info, const char* path);