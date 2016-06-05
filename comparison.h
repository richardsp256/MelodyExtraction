#include "sndfile.h"
#include "fftw3.h"

void PrintAudioMetadata(SF_INFO * file);
float* WindowFunction(int size);
int ExtractMelody(char* filename);
int STFT(double** signal, SF_INFO info, int blocksize, double*** dft_data);