#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "calcSummedLagCorrentrograms.h"
#include "rollSigma.h"
#include "gammatoneFilter.h"
#include "../errors.h"
#include "../utils.h" // AlignedAlloc

#define MAX_CHANNELS 128

static int simpleComputePSM(int numChannels, const float * restrict data,
			    float * restrict buffer,
			    const float * restrict centralFreq,
			    int sampleRate, int dataLength, int startIndex,
			    int interval, float scaleFactor, int sigWindowSize,
			    int numWindows, float * restrict sigmas,
			    int correntropyWinSize,
			    float * restrict pooledSummaryMatrix)
{
	float loop_time = 0.0f;
	float correntropy_time = 0.0f;
	for (int i = 0;i<numChannels;i++){
		clock_t entry_c = clock();

		int tmp_rslt;

		tmp_rslt = sosGammatone(data, buffer, centralFreq[i],
					sampleRate, dataLength, NULL);
		if (tmp_rslt != ME_SUCCESS){
			return tmp_rslt;
		}

		/* compute the sigma values */
		rollSigma(startIndex, interval, scaleFactor, sigWindowSize,
			  dataLength, numWindows, buffer, sigmas);

		clock_t precorrentrogram_c = clock();
		/* compute the pooledSummaryMatrixValues */
		tmp_rslt = CalcSummedLagCorrentrograms(
			buffer, sigmas, (size_t) correntropyWinSize,
			(size_t) correntropyWinSize, (size_t) interval,
			(size_t) numWindows, pooledSummaryMatrix);
		if (tmp_rslt){
			return tmp_rslt;
		}

		clock_t exit_c = clock();
		loop_time += ((float)(exit_c-entry_c))/CLOCKS_PER_SEC;
		correntropy_time +=
			((float)(exit_c-precorrentrogram_c))/CLOCKS_PER_SEC;
	}
	printf("  average correntropy time (seconds): %e\n",
	       correntropy_time / numChannels);
	printf("  average total loop time (seconds):  %e\n",
	       loop_time / numChannels);
	return 0;
}

static int computeNumWindows(int dataLength, int correntropyWinSize,
			     int interval)
{
	int numWindows = (int)ceil((dataLength - correntropyWinSize)/
				   (float)interval) + 1;
	return numWindows;
}

int computeDetFunctionLength(int dataLength, int correntropyWinSize,
			     int interval)
{
	return computeNumWindows(dataLength, correntropyWinSize, interval) - 1;
}

int CalcDetFunc(int correntropyWinSize, int interval, float scaleFactor,
		int sigWindowSize, int numChannels, float minFreq,
		float maxFreq, int sampleRate, int audioLength,
		const float * audio, int detFunctionLength, float* detFunction)
{
	// TODO Refactor so that this function doesn't allocate memory from the
	//      heap (i.e. use a fixed buffer allocated on the stack)

	if (numChannels > MAX_CHANNELS || numChannels < 1) {
		return ME_BAD_NUM_FILTER_CHANNELS;
	}
	float centralFreq[MAX_CHANNELS];
	centralFreqMapper(numChannels, minFreq, maxFreq, centralFreq);

	// check correntrogram properties. Doing this now ensures that we
	// compute a buffer size that is an integral multiple of 4
	int arg_check = CheckCorrentrogramsProp((size_t) correntropyWinSize,
						(size_t) correntropyWinSize,
						(size_t) interval);
	if (arg_check != 0){
		return arg_check;
	}

	int numWindows = computeNumWindows(audioLength, correntropyWinSize,
					   interval);
	printf("audiolen: %d, numWindows: %d\n", audioLength, numWindows);
	if (detFunctionLength != (numWindows-1)){
		return ME_BAD_DETECTION_FUNCTION_LEN;
	}

	// For performance reasons, CalcSummedLagCorrentrograms doesn't process
	// filterred audio with arbitrary lengths. It requires that the buffer
	// holds the following number of entries:
	int bufferLength = (numWindows-1)*interval + 2*correntropyWinSize;
	printf("bufferLength: %d\n",bufferLength);
	float * buffer = AlignedAlloc(16, sizeof(float)*bufferLength);
	// We accomodate this by simply setting all elements with indices >=
	// audioLength to zero.
	// TODO: Improve handling of alternative lengths. It would be more
	//       correct to pad the end of the input signal with 0s
	if (buffer){
		for (int i = audioLength; i < bufferLength; i++){
			buffer[i] = 0;
		}
	} else {
		return ME_MALLOC_FAILURE;
	}

	// Prepare buffers for pooledSummaryMatrix and sigmas
	float * pooledSummaryMatrix = calloc(numWindows, sizeof(float));
	float * sigmas = malloc(sizeof(float)*numWindows);

	if ((!pooledSummaryMatrix) || (!sigmas)){
		free(buffer);
		if (pooledSummaryMatrix){
			free(pooledSummaryMatrix);
		}
		if (sigmas){
			free(sigmas);
		}
		return ME_MALLOC_FAILURE;
	}

	// compute startIndex (for rollSigma function)
	int startIndex = correntropyWinSize/2;

	int result = simpleComputePSM(numChannels, audio, buffer, centralFreq,
				      sampleRate, audioLength, startIndex,
				      interval, scaleFactor, sigWindowSize,
				      numWindows, sigmas, correntropyWinSize,
				      pooledSummaryMatrix);
	if (result == 0){
		for (int i = 0; i < detFunctionLength; i++){
			detFunction[i] = (pooledSummaryMatrix[i+1]
					  - pooledSummaryMatrix[i]);
		}
	}

	free(sigmas);
	free(buffer);
	free(pooledSummaryMatrix);
	return result;
}
