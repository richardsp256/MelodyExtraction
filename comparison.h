#include "sndfile.h"
#include "fftw3.h"

void PrintAudioMetadata(SF_INFO * file);
int ReadAudioFile(char* filename, int blocksize, double*** dft_data, unsigned int* samplerate, unsigned int* frames);