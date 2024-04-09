#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "filterBank.h"
#include "calcSummedLagCorrentrograms.h"
#include "../utils.h" // AlignedAlloc

#define MAX_CHANNELS 128

/* Concept is taken from python Pandas package*/

/* If the window is centred, then curLocation refers to the centre of the 
   window. Otherwise, it refers to the left edge */

struct windowIndexer{
	int variable; /* expects 1 or 0 */
	int centered; /* expects 1 or 0 */
	int sizeLeft;
	int sizeRight;
	int interval;
	int curLocation; 
	int arrayLength;
};


struct windowIndexer* windowIndexerNew(int variable, int centered, int winSize,
				       int interval, int startIndex,
				       int arrayLength)
{
	struct windowIndexer* out;
	out = malloc(sizeof(struct windowIndexer));
	out->variable = variable;
	out->centered = centered;
	if (centered){
		out->sizeLeft = winSize/2;
		out->sizeRight = out->sizeLeft + 1;
		if (winSize % 2 == 0){
			(out->sizeLeft)-=1;
		}
	} else {
		out->sizeLeft = 0;
		out->sizeRight = winSize;
	}
	out->interval = interval;
	out->curLocation = startIndex;
	out->arrayLength = arrayLength;
	return out;
}

void windowIndexerDestroy(struct windowIndexer *wInd){
	free(wInd);
}

void wIndAdvance(struct windowIndexer *wInd){
	(wInd->curLocation) += (wInd->interval);
}

int wIndGetStart(struct windowIndexer *wInd){
	if (wInd->centered !=0){
		int temp;
		temp = wInd->curLocation - wInd->sizeLeft;
		if (temp < 0){
			// applies to both variable and non-variable windows
			return 0;
		}else{
			return temp;
		}
	} else {
		return (wInd->curLocation);
	}
}

int wIndGetStop(struct windowIndexer *wInd){
	int temp = wInd->curLocation + wInd->sizeRight;
	
	if (wInd->variable){
		if (temp > wInd->arrayLength){
			return wInd->arrayLength;
		} else {
			return temp;
		}
	} else {
		return temp;
	}
}


/* the following draws HEAVY inspiration from implementation of pandas 
 * roll_variance. 
 * https://github.com/pandas-dev/pandas/blob/master/pandas/_libs/window.pyx
 */
static inline double calc_var(double nobs, double ssqdm_x){
	double result;
	/* Variance is unchanged if no observation is added or removed
	 */
	if (nobs == 1){
		result = 0;
	} else {
		result = ssqdm_x / (nobs - 1);
		if (result < 0){
			result = 0;
		}
	}
	return result;
}

static inline void add_var(double val, int *nobs, double *mean_x,
			   double *ssqdm_x){
	/* add a value from the var calc */
	double delta;

	(*nobs) += 1;
	delta = (val - *mean_x);
	(*mean_x) += delta / *nobs;
	(*ssqdm_x) += (((*nobs-1) * delta * delta) / *nobs);
}

static inline void remove_var(double val, int *nobs, double *mean_x,
			      double *ssqdm_x){
	/* remove a value from the var calc */

	double delta;

	(*nobs) -= 1;
	if (*nobs>0){
		delta = (val - *mean_x);
		(*mean_x) -= delta / *nobs;
		(*ssqdm_x) -= (((*nobs+1) * delta * delta) / *nobs);
	} else {
		*mean_x = 0;
		*ssqdm_x = 0;
	}
}

/// Compute the rolling variance following the algorithm from the pandas python
/// package.
///
/// This algorithm can still be further optimized so that we add
/// and remove values from the windows in chunks (rather than 1 at a time).
void rollSigma(int startIndex, int interval, float scaleFactor,
	       int sigWindowSize, int dataLength, int numWindows,
	       float *buffer, float *sigmas)
{
	int i,j,k,nobs = 0,winStart, winStop, prevStop,prevStart;
	double mean_x = 0, ssqdm_x = 0;
	float std;
	struct windowIndexer* wInd;

	wInd = windowIndexerNew(1, 1, sigWindowSize, 1,
				startIndex, dataLength);
	
	for (i=0;i<numWindows;i++){
		winStart = wIndGetStart(wInd);
		winStop = wIndGetStop(wInd);
		/* Over the first window observations can only be added 
		 * never removed */
		if (i == 0){
			for (j=winStart;j<winStop;j++){
				//printf("%f\n",buffer[j]);
				add_var((double)buffer[j], &nobs, &mean_x,
					&ssqdm_x);
			}
		} else {
			/* after the first window, observations can both be 
			 * added and removed */

			/* calculate adds 
			 * (almost always iterates over 1 value, for the final
			 *  windows, iterates over 0 values)
			 */
			for (j=prevStop;j<winStop;j++){
				add_var((double)buffer[j], &nobs, &mean_x,
					&ssqdm_x);
			}

			/* calculate deletes 
			 * (should always iterates over 1 value)
			 */
			for (j = prevStart; j<winStart; j++){
				remove_var((double)buffer[j], &nobs, &mean_x,
					   &ssqdm_x);
			}
		}
		wIndAdvance(wInd);
		prevStart = winStart;
		prevStop = winStop;

		/* compute sigma */
		std = sqrtf((float)calc_var(nobs, ssqdm_x));
		sigmas[i] = (scaleFactor*std/powf((float)nobs,0.2));

		/* Now we need to advance the window over the interval between  
		 * the current where we just calculated sigma and the next 
		 * location where we want sigma. 
		 * We do this because the windowIndexer only advances one index 
		 * at a time and we are only interested in keeping the sigma 
		 * values separated by interval (a number indices typically 
		 * greater than 1) */
		if (i == (numWindows-1)){
			break;
		}

		for (k = 1; k<interval;k++){
			winStart = wIndGetStart(wInd);
			winStop = wIndGetStop(wInd);
			/* after the first window, observations can both be 
			 * added and removed */

			/* calculate adds */
			for (j=prevStop;j<winStop;j++){
				add_var((double)buffer[j], &nobs, &mean_x, &ssqdm_x);
			}

			/* calculate deletes */
			for (j = prevStart; j< winStart;j++){
				remove_var((double)buffer[j], &nobs, &mean_x, &ssqdm_x);
			}
			wIndAdvance(wInd);
			prevStart = winStart;
			prevStop = winStop;
		}
	}
	windowIndexerDestroy(wInd);
}

int simpleComputePSM(int numChannels, const float * restrict data,
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

		//sosGammatone(data, buffer, centralFreq[i], sampleRate,
		//	       dataLength);
		sosGammatoneFast(data, buffer, centralFreq[i], sampleRate,
			         dataLength);

		/* compute the sigma values */
		rollSigma(startIndex, interval, scaleFactor, sigWindowSize,
			  dataLength, numWindows, buffer, sigmas);

		clock_t precorrentrogram_c = clock();
		/* compute the pooledSummaryMatrixValues */
		CalcSummedLagCorrentrograms(buffer, sigmas,
					    (size_t) correntropyWinSize,
					    (size_t) correntropyWinSize,
					    (size_t) interval,
					    (size_t) numWindows,
					    pooledSummaryMatrix);

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

int computeNumWindows(int dataLength, int correntropyWinSize, int interval)
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

int simpleDetFunctionCalculation(int correntropyWinSize, int interval,
				 float scaleFactor, int sigWindowSize,
				 int numChannels, float minFreq, float maxFreq,
				 int sampleRate, int dataLength, float* data,
				 int detFunctionLength, float* detFunction)
{

	// TODO change the type of all length/index variables to size_t
	// TODO Refactor so that this function doesn't allocate memory from the
	//      heap (i.e. use a fixed buffer allocated on the stack)
	// TODO Add meaningful error codes

	if (numChannels > MAX_CHANNELS) {
		return -1;
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

	int numWindows = computeNumWindows(dataLength, correntropyWinSize,
					   interval);
	printf("datalen: %d, numWindows: %d\n", dataLength, numWindows);
	if (detFunctionLength != (numWindows-1)){
		return -1;
	}

	// For performance reasons, CalcSummedLagCorrentrograms doesn't process
	// filterred data with arbitrary lengths. It requires that the buffer
	// holds the following number of entries:
	int bufferLength = (numWindows-1)*interval + 2*correntropyWinSize;
	printf("bufferLength: %d\n",bufferLength);
	float * buffer = AlignedAlloc(16, sizeof(float)*bufferLength);
	// We accomodate this by simply setting all elements with indices >=
	// dataLength to zero.
	// TODO: Improve handling of alternative lengths. It would be more
	//       correct to pad the end of the input signal with 0s
	for (int i = dataLength; i < bufferLength; i++){
		buffer[i] = 0;
	}

	// Prepare buffer for pooledSummaryMatrix
	float * pooledSummaryMatrix = malloc(sizeof(float)*numWindows);
	for (int i = 0; i < numWindows; i++){
		pooledSummaryMatrix[i] = 0;
	}

	float * sigmas = malloc(sizeof(float)*numWindows);
	int startIndex = correntropyWinSize/2;

	int result = simpleComputePSM(numChannels, data, buffer, centralFreq,
				      sampleRate, dataLength, startIndex,
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
