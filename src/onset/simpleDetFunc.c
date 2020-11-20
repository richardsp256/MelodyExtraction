#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "filterBank.h"

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

/* Basically we compute the rolling variance following the algorithm from 
 * python pandas. This algorithm can still be further optimized so that we add
 * and remove values from the windows in chunks (rather than 1 at a time). 
 * Additionally, if we store wInd on the stack, we might get better 
 * optimizations.
 */

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


/**
Below function attempts to be optimized for speed AS MUCH AS POSSIBLE.
The overwhelming majority (> 90%) of the programs runtime is within this function.
We approximate e^x based off this method for doubles: https://nic.schraudolph.org/pubs/Schraudolph99.pdf
A variation for floats taken from https://stackoverflow.com/questions/9652549/self-made-pow-c
Below is a simplified, much slower version of what below function does, for clarity:

static inline float calcPSMEntryContrib(float* x, int window_size, float sigma)
{
	float out = 0;
	for (int i=0; i<window_size; i++){
		for (int j=1; j<=window_size; j++){
			float temp = x[i] - x[i+j];
			out += expf(-1 * temp * temp);
		}
	}
	return out / (sigma * sqrt(2* M_PI));
}
**/
#define EXP_A 184
#define EXP_C 16249 
#define M_1_SQRT2PI 0.3989422804f
#define EXP_UPPER_BOUND 9.345f
static inline float calcPSMEntryContrib(float* x, int window_size, float sigma)
{
	//union allows us to treat the same 4 bytes of memory as both a float and 2 shorts
	union {
		float f;
		struct {
			short j, i; //if on big-endian architecture, j, i must be flipped to i, j
		} s;
	} res;
	int i, j;
	float out, temp;
	res.s.j = 0;
	out = 0;
	for (i = 0; i < window_size; i++) {
		for (j = 1; j <= window_size; j++) {
			temp = x[i] - x[i+j];
			//for values of temp greater in magnitude than 9.345, e^(-(temp^2)) is smaller than FLT_MIN
			//expf() would return 0.0f for results smaller than FLT_MIN, but EXP_APPROX will return NaN,-NaN for most (not all, some underflow)
			//by only computing values in valid range, ones outside are treated as 0.
			if(fabsf(temp) < EXP_UPPER_BOUND){
				res.s.i = EXP_A*(-1.0f * temp * temp) + (EXP_C);
				out += res.f;
			}
		}
	}
	out *= M_1_SQRT2PI / sigma;
	return out;
}

void pSMContribution(int correntropyWinSize, int interval, int numWindows,
		     float *buffer, float *sigmas, float *pSMatrix)
{
	int i,start,j,calcBufferLength;
	float *calcBuffer, denom;
	start = 0;

	calcBufferLength = 2*correntropyWinSize +1;
	calcBuffer = malloc(sizeof(float)*calcBufferLength);

	for (i=0;i<numWindows;i++){
		// not sure if the following function will work correctly:
		denom = M_SQRT1_2/sigmas[i];

		/* The fact that that the first entry in a window is not 
		 * being placed in the calculation buffer for the correntropy 
		 * calculation. Per the paper, the correntropy calculation uses 
		 * the 2*correntropyWinSize elements immediately following the 
		 * first entry in the window
		 */
		for (j=1;j<=calcBufferLength;j++){
			calcBuffer[j] = (buffer[start+j]) * denom;
		}
		pSMatrix[i] += calcPSMEntryContrib(calcBuffer,
						   correntropyWinSize,
						   sigmas[i]);
		//if (i>=1400){
		//	printf("Current pSMatrix[%d] value: %f\n",
		//	       i,(*pSMatrix)[i]);
		//}
		start+=interval;
	}
	free(calcBuffer);
}

void simpleComputePSM(int numChannels, float* data, float **buffer,
		      float *centralFreq, int sampleRate, int dataLength,
		      int startIndex, int interval, float scaleFactor,
		      int sigWindowSize, int numWindows, float *sigmas,
		      int correntropyWinSize, float **pooledSummaryMatrix)
{
	float averageTime = 0.0f;
	for (int i = 0;i<numChannels;i++){
		clock_t c1 = clock();

		//printf("compute channel %d...\n", i);
		//sosGammatone(data, *buffer, centralFreq[i], sampleRate,
		//	       dataLength);
		sosGammatoneFast(data, *buffer, centralFreq[i], sampleRate,
			       dataLength);

		clock_t c2 = clock();

		//printf("   gammatone %d...\n", i);
		/* compute the sigma values */
		rollSigma(startIndex, interval, scaleFactor, sigWindowSize,
			  dataLength, numWindows, *buffer, sigmas);
		//printf("   sigma %d...\n", i);

		clock_t c3 = clock();

		/* compute the pooledSummaryMatrixValues */
		pSMContribution(correntropyWinSize, interval, numWindows,
				*buffer, sigmas, *pooledSummaryMatrix);
		//printf("   matrix %d...\n", i);
		
		clock_t c4 = clock();
		float elapsed = ((float)(c4-c1))/CLOCKS_PER_SEC;
		float elapsed1 = ((float)(c2-c1))/CLOCKS_PER_SEC;
		float elapsed2 = ((float)(c3-c2))/CLOCKS_PER_SEC;
		float elapsed3 = ((float)(c4-c3))/CLOCKS_PER_SEC;
		printf("  %d\telapsed = %0.5f,  (%0.5f,  %0.5f,  %0.5f)\n", i, elapsed*1000, elapsed1*1000, elapsed2*1000, elapsed3*1000);
		averageTime += elapsed;
	}
	printf("  average time: %f\n", (averageTime*1000) / numChannels);
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

	int dFLength,numWindows,bufferLength,i,startIndex;
	float *pooledSummaryMatrix, *sigmas, *centralFreq, *buffer;

	numWindows = computeNumWindows(dataLength, correntropyWinSize,
				       interval);
	printf("datalen: %d, numWindows: %d\n", dataLength, numWindows);
	if (detFunctionLength != (numWindows-1)){
		return -1;
	}

	// The pSMContribution function requires assumes that the filtered data
	// has the following number of entries:
	//   (numWindows-1)*interval + 2*correntropyWinSize + 2
	// For performance reasons, it does not specifically check for and
	// handle alternative lengths, and therefore we need to make sure that
	// the filtered data is stored in a buffer of this length. For
	// simplicity, we just set all elements with indices >= dataLength to
	// zero.
	// (This is something of a legacy solution. It would be more correct to
	// fill in the values for indices >= dataLength assuming that the input
	// signal has values of 0 at these locations).
	bufferLength = (numWindows-1)*interval + 2*correntropyWinSize + 2;
	printf("bufferLength: %d\n",bufferLength);
	buffer = malloc(sizeof(float)*bufferLength);
	for (i = dataLength; i<bufferLength;i++){
		buffer[i] = 0;
	}

	pooledSummaryMatrix = malloc(sizeof(float)*numWindows);
	for (i=0;i<numWindows;i++){
		pooledSummaryMatrix[i] = 0;
	}
	sigmas = malloc(sizeof(float)*numWindows);

	centralFreq = malloc(sizeof(float)*numChannels);
	centralFreqMapper(numChannels, minFreq, maxFreq, centralFreq);

	startIndex = correntropyWinSize/2;

	simpleComputePSM(numChannels, data, &buffer, centralFreq, sampleRate, 
			 dataLength, startIndex, interval, scaleFactor, 
			 sigWindowSize, numWindows, sigmas,
			 correntropyWinSize, 
			 &pooledSummaryMatrix);

	free(sigmas);
	free(buffer);
	for (i = 0; i<detFunctionLength; i++){
		detFunction[i] = (pooledSummaryMatrix[i+1]
				  - pooledSummaryMatrix[i]);
	}
	free(pooledSummaryMatrix);
	free(centralFreq);
	return 1;
}
