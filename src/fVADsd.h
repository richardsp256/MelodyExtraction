#include <stdint.h>

int fVADSilenceDetection(float** AudioData,int sample_rate, int mode,
			 int frameLength, int spacing, int length,
			 int** activityRanges);
void convertSamples(float *inputData, int start, int frameLengthSamples,
		    int16_t *buffer, int length);
int posIntCeilDiv(int x, int y);
int vadHelper(float* data,int sample_rate, int mode, int frameLength,
	      int spacing, int length, int** activityRanges);
int mallocActivityRanges(int** activityRanges, int numFrames);
int reallocActivityRanges(int** activityRanges, int acLength, int numFrames);
void WindowsToSamples(int* windows, int length, int sample_rate);
