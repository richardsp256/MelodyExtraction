#include "fftw3.h"
#include "melodyextraction.h"

float* WindowFunction(int size);
int NumSTFTBlocks(audioInfo info, int unpaddedSize, int interval);
float* Magnitude(const fftwf_complex* arr, int size);
int STFT_r2c(const float* input, audioInfo info, int unpaddedSize, int winSize, int interval, fftwf_complex** fft_data);
int STFTinverse_c2r(const fftwf_complex* input, audioInfo info, int winSize, int interval, float** output);
