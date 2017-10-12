#include "gammatoneFilter.h"

struct filterBank{
	
	struct channelData* cDArray;
	int numChannels; 
	int lenChannels; // in units of number of samples
	int overlap; // in units of number of samples
	int samplerate;
};


struct channelData{
	float cf; // center frequency

	// filter coefficients that must be tracked:
	float *p1r;
	float *p2r;
	float *p3r;
	float *p4r;
	float *p1i;
	float *p2i;
	float *p3i;
	float *p4i;
	float *qcos;
	float *qsin;
};

struct filterBank* filterBankNew(int numChannels, int lenChannels, int overlap,
				 int samplerate, float minFreq, float maxFreq);

void filterBankDestroy(struct filterBank* fB);

float* centralFreqMapper(int numChannels, float minFreq, float maxFreq);

/* The following 3 functions are used for processing inputChunks. The first 
 * function is used when you feed in the very first chunk. The second function
 * is used when you feed in all other chunks except the last chunk. The third 
 * function is used when you feed in the last chunk 
 *
 * for all functions, nsamples is the number of samples in the inputChunk
 */

void filterBankFirstChunk(struct filterBank* fB, float* inputChunk,
			  int nsamples, float** leadingSpectraChunk);

void filterBankUpdateChunk(struct filterBank* fB, float* inputChunk,
			   int nsamples, float** leadingSpectraChunk,
			   float** trailingSpectraChunk);

void filterBankFinalChunk(struct filterBank* fB, float* inputChunk,
			  int nsamples, float** leadingSpectraChunk,
			  float** trailingSpectraChunk);


