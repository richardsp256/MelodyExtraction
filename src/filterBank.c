#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include "filterBank.h"
#include "gammatoneFilter.h"


enum fBstates {
	NO_CHUNK,     // before any chunks have been set in the filterBank
	FIRST_CHUNK,  // The very first chunk has been added - there is no
	              // overlap between buffers
	NORMAL_CHUNK, // The current chunk is neither the first nor the last
	LAST_CHUNK,   // The current chunk is the final chunk - the end of the
	              // chunk must be zero-padded
	SINGLE_CHUNK, // The current chunk is both the first and the last CHUNK
};

/* we may refactor the way in which do the biquad filtering so that we compute 
 * values element by element rather than recursively applying the filter to one 
 * full chunk at a time.
 */
struct channelData{
	float cf; // center frequency
	//int num_stages; - should always be 4
	double coef[24];  // coefficients for all stages of the biquad filter
	               // this always has a length = num_stages*6
	               // Coefficients for section i of the biquad filter are
	               // found at:    (coef + i*6)
	               // The order of the coefficients are:
	               //     b0, b1, b2, a0, a1, and a2
	double state[8]; // this array includes the state variables d1 and d2
	               // Its size is 2*num_stages
	               // All entries are initially set to 0.
	               // State variable for section i of the biquad filter are
	               // found at:    (state + i*2)
	               // The order of the coefficients are:
	               //     d1, d2	
};

typedef int (*fBChunkProcessFunc)(filterBank* fB, tripleBuffer* tB,
				  struct channelData *cD, int channel);
// the following are functions tracked internally by filterbank based on its
// state
// They explain how to process the various chunks in each state.
int filterBankProcessNoChunk(filterBank* fB, tripleBuffer* tB,
			     struct channelData *cD, int channel);
int filterBankProcessFirstChunk(filterBank* fB, tripleBuffer* tB,
				struct channelData *cD, int channel);
int filterBankProcessNormalChunk(filterBank* fB, tripleBuffer* tB,
				 struct channelData *cD, int channel);
int filterBankProcessLastChunk(filterBank* fB, tripleBuffer* tB,
			       struct channelData *cD, int channel);
int filterBankProcessSingleChunk(filterBank* fB, tripleBuffer* tB,
				 struct channelData *cD, int channel);

struct filterBank{
	
	struct channelData* cDArray;
	int numChannels; 
	int lenChannels; // in units of number of samples
	int overlap; // in units of number of samples
	int samplerate;
	int terminationIndex; // this is the stop index of the final inputChunk
	float *inputChunk;
	enum fBstates state;
	fBChunkProcessFunc chunkProcessFunc;
};




filterBank* filterBankNew(int numChannels, int lenChannels, int overlap,
			  int samplerate, float minFreq, float maxFreq)
{
	struct filterBank* fB;
	float *fcArray;
	
	if (numChannels<1){
		printf("Error: numChannels must be postive\n");
		return NULL;
	}
	if (lenChannels<1){
		printf("Error: lenChannels must be postive\n");
		return NULL;
	}
	if (overlap<0){
		printf("Error: overlap must be >=0\n");
		return NULL;
	}
	if (samplerate<1){
		printf("Error: samplerate must be postive\n");
		return NULL;
	}
	if (minFreq<=0){
		printf("Error: minFreq must be >= 0\n");
		return NULL;
	}
	if (maxFreq<minFreq){
		printf("Error: maxFreq must be >= minFreq\n");
		return NULL;
	}
	if ((maxFreq==minFreq) && (numChannels!=1)){
		printf("Error: if maxFreq == minFreq, numChannels must be 1\n");
		return NULL;
	}
	
	fB = malloc(sizeof(struct filterBank));
	if (fB == NULL){
		return fB;
	}
	(fB->cDArray) = malloc(sizeof(struct channelData)*numChannels);

	if ((fB->cDArray) == NULL){
		free(fB);
		return fB;
	}
	//printf("Constructing the channel Data array\n");
	fcArray = centralFreqMapper(numChannels, minFreq, maxFreq);
	if (fcArray == NULL){
		filterBankDestroy(fB);
		return NULL;
	}

	for (int i=0; i<numChannels; i++){
		// fill in the channel data
		struct channelData *cD = (fB->cDArray) +i;
		cD->cf = fcArray[i];
		sosCoef(fcArray[i], samplerate, cD->coef);
		for (int j=0; j<8;j++){
			cD->state[j]=0;
		}
	}

	fB->numChannels = numChannels;
	fB->lenChannels = fB->lenChannels;
	fB->overlap = overlap;
	fB->samplerate = samplerate;
	fB->terminationIndex = -1;
	fB->inputChunk = NULL;
	fB->state = NO_CHUNK;
	fB->chunkProcessFunc = filterBankProcessNoChunk;

	free(fcArray);
	
	return fB;
}

void filterBankDestroy(filterBank* fB){
	free(fB->cDArray);
	free(fB);
}


float* centralFreqMapper(int numChannels, float minFreq, float maxFreq){
	/* The paper states that the frequencies are mapped according to the 
	 * according to the Equivalent Rectangular Bandwifth (ERB) scale.
	 * According to wikipedia, 
	 * https://en.wikipedia.org/wiki/Equivalent_rectangular_bandwidth,
	 * a linear approximation of ERB between 100 and 10000 Hz, is given by
	 * ERB = 24.7 * (4.37 * f + 1), where ERB is in units of Hz, and f is 
	 * in units in kHz. We can rewrite this equation where f is in Hz as:
	 * ERB = 24.7 * (0.00437 * f + 1).
	 * We will use this linear approximation since it is the same as the 
	 * one used by our gammatone filter.
	 *
	 * According to the same page, the ERB scale (ERBS) for this linear 
	 * approximation is given by:
	 * ERBS = 21.3 * log10(1+0.00437*f) where f has units of Hz.
	 * We calculate the inverse to be:
	 * f = (10^(ERBS/21.4) - 1)/0.00437
	 *
	 * Onto the problem at hand. we will return an array of central 
	 * frequencies, fcArray, of length = numChannels.
	 *
	 * If numChannels == 1, then the only entry will be minFreq
	 *
	 * In general, the minimum and maximum 
	 * frequencies (fmin and fmax) included in a bandwidth B, centred on 
	 * fc are given by: fmin = fc - B/2 and fmax = fc + B/2
	 * In function calls problems where there are at least 2 channels, 
	 * we will choose the very first and the very last central frequencies 
	 * such that the minFreq and maxFreq are included in the edge of the 
	 * bandwidths. In other words: 
	 * minFreq = min(fcArray) - B/2 and maxFreq = max(fcArray) + B/2
	 * plugging in ERB = 24.7 * (0.00437 * fc + 1) for B in each equation 
	 * we can solve for min(fcArray) and max(fcArray):
	 * min(fcArray) = (minFreq + 12.35)/0.9460305
	 * max(fcArray) = (maxFreq - 12.35)/1.0539695
	 *
	 * then I will space out the remaining central frequencies with 
	 * evenly with respect to ERBS
	 * Let minERBS = ERBS(min(fcArray)) and maxERBS = ERBS(max(fcArray))
	 * The ith entry of fcArray will have ERBS given by
	 * ERBS(fcArray[i]) = minERBS + i * (maxERBS-minERBS)/(numChannels - 1)
	 *
	 * We can calculate the ith entry of fcArray by taking the inverse of 
	 * the ERBS of the ith entry
	 * fcArray[i] =(10.^((minERBS + i * (maxERBS-minERBS)/(numChannels - 1))
	 *                   /21.4) - 1)/0.00437
	 */
	float *fcArray, minERBS,maxERBS;
	int i;
	
	fcArray = malloc(sizeof(float)*numChannels);
	if (fcArray ==NULL){
		return NULL;
	}

	if (numChannels == 1){
		fcArray[0] = minFreq;
		return fcArray;
	}
	
	/* calculate the minimum central frequency */
	fcArray[0] = (minFreq + 12.35)/0.9460305;

	/* calculate the maximum central frequency */
	fcArray[numChannels-1] = (maxFreq - 12.35)/1.0539695;

	/* calculate minERBS and maxERBS */
	minERBS = 21.3 * log10f(1+0.00437*fcArray[0]);
	maxERBS = 21.3 * log10f(1+0.00437*fcArray[numChannels-1]);

	/* calculate all other entries of fcArray */
	for (i=1;i<numChannels-1;i++){
		fcArray[i] = ((powf(10.,((minERBS + (float)i * (maxERBS-minERBS)
					  /((float)numChannels - 1))/21.4))
			       - 1.0) /0.00437);
	}
	return fcArray;
}

int filterBankFirstChunkLength(filterBank *fB){
	return fB->lenChannels;
}

int filterBankNormalChunkLength(filterBank *fB){
	return fB->lenChannels - fB->overlap;
}

int filterBankSetInput(filterBank* fB, float * input, int length,
		       int final_chunk){
	if (length < 0){
		return -1;
	} else if ((final_chunk != 0) || final_chunk != 1) {
		return -2;
	}

	switch(fB->state) {
	case LAST_CHUNK :
		return -3;
	case SINGLE_CHUNK :
		return -3;
	case NO_CHUNK :
		/* do stuff - can only become FIRST_CHUNK or SINGLE_CHUNK */
		if (final_chunk == 0){
			if (length != filterBankFirstChunkLength(fB)){
				return -4;
			}
			fB->state = FIRST_CHUNK;
			fB->chunkProcessFunc = filterBankProcessFirstChunk;
		} else {
			if (length > filterBankFirstChunkLength(fB)){
				return -5;
			}
			fB->state = SINGLE_CHUNK;
			fB->chunkProcessFunc = filterBankProcessSingleChunk;
		}
		break;
	default :
		/* FIRST_CHUNK or NORMAL_CHUNK
		 * in either case,  can only become NORMAL_CHUNK or LAST_CHUNK
		 */
		if (final_chunk == 0){
			if (length != filterBankNormalChunkLength(fB)){
				return -4;
			}
			fB->state = NORMAL_CHUNK;
			fB->chunkProcessFunc = filterBankProcessNormalChunk;
		} else {
			if (length > filterBankNormalChunkLength(fB)){
				return -5;
			}
			fB->state = LAST_CHUNK;
			fB->chunkProcessFunc = filterBankProcessLastChunk;
		}
	}
	fB->inputChunk = input;
	if (final_chunk == 1){
		fB->terminationIndex = length;
	}
	return 1;
}

int filterBankProcessInput(filterBank *fB, tripleBuffer *tB, int channel){
	if (channel < 0){
		return -1;
	} else if (channel >= fB->numChannels){
		return -2;
	} else if (tB == NULL){
		return -3;
	}
	struct channelData *cD = fB->cDArray + channel;
	
	return (fB->chunkProcessFunc)(fB, tB, cD, channel);
}

// we can remove samplerate as an argument from this function.
void filteringHelper(struct channelData *cD, float* inputChunk,
		     float* filteredChunk, int nsamples, int samplerate){
	/* use the gammatone filter on the first nsamples of inputChunk and 
	 * saving the output in the first nsamples in filteredChunk.
	 *
	 * This implementation assumes 4 biquad filters.
	 * the ith row of the cD->coef contains b0, b1, b2, a0, a1, and a2 
	 * while the ith row of the cD->state contains d1, d2
	 *
	 * The complexity could be improved if we force a0 = 1
	 * In order to avoid denormal numbers, we might need to multiply by 
	 * Gain
	 */
	
	for (int k=0; k<nsamples; k++){
		/* below we will process 4 recursive filters 
		 * the input into the i=0 filter is x[k] */
		double result = inputChunk[k];
		for (int i = 0; i <4; i++){
			double input = result;
			/* y[n] = (b0 * x[n] + d1[n-1])/a0;
			 */
			result = (((cD->coef)[i*6 + 0] * input +
				   (cD->state)[i*2 + 0]) / (cD->coef)[i*6+3]);
			/* next difference equation: 
			 *  d1[n] = b1 * x[n] - a1 * y[n] + d2[n-1]
			 */
			(cD->state)[i*2 + 0] = ((cD->coef)[i*6 + 1] * input -
						(cD->coef)[i*6 + 4] * result +
						(cD->state)[i*2 + 1]);

			/* final difference equation:
			 *  d2[n] = b2 * x[n] - a2 * y[n]
			 */
			(cD->state)[i*2 + 1] = ((cD->coef)[i*6 + 2] * input -
						(cD->coef)[i*6 + 5] * result);

		}
		filteredChunk[k] = (float)result;
	}
}

void overlapHelper(float* leadingSpectraChunk, float* trailingSpectraChunk,
		   int nsamples){
	for (int i=0;i<nsamples;i++){
		leadingSpectraChunk[i] = trailingSpectraChunk[i];
	}
}

void zeroPaddingHelper(float* filteredChunk, int nsamples){
	for (int i=0;i<nsamples;i++){
		filteredChunk[i] = 0.f;
	}
}

int processFirstChunkHelper(filterBank* fB, tripleBuffer* tB,
			    struct channelData *cD, int channel,
			    int inputLength){
	// this consolidates implementations of filterBankProcessFirstChunk and
	// filterBankProcessSingleChunk. To achieve the former set inputLength
	// to lenChannels. To achieve the latter set it to terminationIndex
	if (tripleBufferNumBuffers(tB) != 1){
		return -5;
	}
	float *filteredChunk = tripleBufferGetBufferPtr(tB, 0, channel);
	if (filteredChunk == NULL){
		return -6;
	}
	float *inputChunk = fB->inputChunk;
	int nsamples = fB->lenChannels;
	int samplerate = fB->samplerate;
	
	filteringHelper(cD, inputChunk, filteredChunk, inputLength, samplerate);
	if (inputLength<nsamples){
		zeroPaddingHelper(filteredChunk+inputLength,
				  nsamples-inputLength);
	}
	return 1;
}

int processOverlappingBufferHelper(filterBank* fB, tripleBuffer* tB,
				   struct channelData *cD, int channel,
				   int inputLength){
	/* This helper function consolidates implementation for 
	 * filterBankProcessNormalChunk and filterBankProcessLastChunk. To 
	 * achieve the former, set inputLength to lenChannels - overlap. To 
	 * achieve the latter, set it to terminationIndex */
	int num_buffer = tripleBufferNumBuffers(tB);
	if (num_buffer <2){
		return -7;
	}
	float *curLeadingBuf = tripleBufferGetBufferPtr(tB, num_buffer-1,
							channel);
	float *curTrailingBuf = tripleBufferGetBufferPtr(tB, num_buffer-2,
							 channel);
	if ((curLeadingBuf == NULL) || (curTrailingBuf == NULL)){
		return -6;
	}
	int nsamples = fB->lenChannels;
	
	int overlap = fB->overlap;

	// copy the overlapping regions
	overlapHelper(curLeadingBuf, curTrailingBuf + (nsamples - overlap),
		      overlap);

	float *inputChunk = fB->inputChunk;
	int samplerate = fB->samplerate;

	// do the filtering
	filteringHelper(cD, inputChunk, curLeadingBuf+overlap, inputLength,
			samplerate);

	// possibly zero-pad
	if ((inputLength+overlap)<nsamples){
		int offset = inputLength+overlap;
		zeroPaddingHelper(curLeadingBuf+offset, nsamples-offset);
	}
	return 1;
}

/* define 5 fBChunkProcessFunc functions
 * The one for NO_CHUNK is supposed to be really simple - Returns -1 for failure
 * The remaining 4 functions should all be built from some combination of 3
 * helper functions and all should be very simple.
 */

int filterBankProcessNoChunk(filterBank* fB, tripleBuffer* tB,
			     struct channelData *cD, int channel) {
	return -4;
}

int filterBankProcessFirstChunk(filterBank* fB, tripleBuffer* tB,
				struct channelData *cD, int channel){
	return processFirstChunkHelper(fB, tB, cD, channel, fB->lenChannels);
}

int filterBankProcessNormalChunk(filterBank* fB, tripleBuffer* tB,
				 struct channelData *cD, int channel){
	/* This processes a normal input chunk - the chunk is of the correct 
	 * length and there is overlap with the previous chunk */
	return processOverlappingBufferHelper(fB, tB, cD, channel,
					      (fB->lenChannels - fB->overlap));
}

int filterBankProcessLastChunk(filterBank* fB, tripleBuffer* tB,
			       struct channelData *cD, int channel){
	return processOverlappingBufferHelper(fB, tB, cD, channel,
					      fB->terminationIndex);
}

int filterBankProcessSingleChunk(filterBank* fB, tripleBuffer* tB,
				 struct channelData *cD, int channel){
	/* there will only be one chunk in the entire data stream. This 
	 * function is currently processing it */
	return processFirstChunkHelper(fB, tB, cD, channel,
				       fB->terminationIndex);
}


int filterBankPropogateFinalOverlap(filterBank *fB, tripleBuffer *tB,
				    int channel){
	if (channel < 0){
		return -1;
	} else if (channel >= fB->numChannels){
		return -2;
	} else if (tB == NULL){
		return -3;
	}
	struct channelData *cD = fB->cDArray + channel;
	
	if ((fB->state != LAST_CHUNK) && (fB->state != SINGLE_CHUNK)){
		return -4;
	} else if ((fB->terminationIndex) <= filterBankNormalChunkLength(fB)){
		return -5;
	}

	processOverlappingBufferHelper(fB, tB, cD, channel,0);
	return 1;
}
