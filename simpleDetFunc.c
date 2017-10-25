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
				       int arrayLength){
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
	(*ssqdm_x) += (delta * (val - *mean_x));
}

static inline void remove_var(double val, int *nobs, double *mean_x,
			      double *ssqdm_x){
	/* remove a value from the var calc */

	double delta;

	(*nobs) += 1;
	if (*nobs>0){
		delta = (val - *mean_x);
		(*mean_x) -= delta / *nobs;
		(*ssqdm_x) -= (delta * (val - *mean_x));
	} else {
		*mean_x = 0;
		*ssqdm_x = 0;
	}
}

/* Basically we compute the rolling variance following the algorithm from 
 * python pandas. (This algorithm can still be further optimized to be much 
 * more similar to the standard deviation algorithm used for the alternative 
 * detection function calculation. (It just requires a more careful 
 * implementation that I don't currently have the patience for).
 */

void rollSigma(int startIndex, int interval, float scaleFactor,
	       int sigWindowSize, int dataLength, int numWindows,
	       float *buffer, float **sigmas){
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
				add_var(buffer[j], &nobs, &mean_x, &ssqdm_x);
			}
		} else {
			/* after the first window, observations can both be 
			 * added and removed */

			/* calculate adds 
			 * (almost always iterates over 1 value, for the final
			 *  windows, iterates over 0 values)
			 */
			for (j=prevStop;j<winStop;j++){
				add_var(buffer[j], &nobs, &mean_x, &ssqdm_x);
			}

			/* calculate deletes 
			 * (should always iterates over 1 value)
			 */
			for (j = prevStart; j<winStart; j++){
				remove_var(buffer[j], &nobs, &mean_x, &ssqdm_x);
			}
		}
		wIndAdvance(wInd);
		prevStart = winStart;
		prevStop = winStop;

		/* compute sigma */
		std = sqrtf((float)calc_var(nobs, ssqdm_x));
		(*sigmas)[i] = (scaleFactor*std/powf((float)nobs,0.2));

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
				add_var(buffer[j], &nobs, &mean_x, &ssqdm_x);
			}

			/* calculate deletes */
			for (j = prevStart; j< winStart;j++){
				remove_var(buffer[j], &nobs, &mean_x, &ssqdm_x);
			}
			wIndAdvance(wInd);
			prevStart = winStart;
			prevStop = winStop;

		}
	}
	windowIndexerDestroy(wInd);	
}





#define M_1_SQRT2PI 0.3989422804
static inline float calcPSMEntryContrib(float* x, int window_size, float sigma)
{
	int i,j;
	double out,temp;
	out=0;
	for (i=0;i<window_size;i++){
		for (j=1; j<=window_size;j++){
			temp = x[i]- x[i+j]; 
			out+=exp(-1.0 * temp * temp);
		}
	}
	out *= M_1_SQRT2PI / sigma;
	//printf("out = %f\n",(float)out);
	return (float)out;
}

void pSMContribution(int correntropyWinSize, int interval,
		     int numWindows, float *buffer,
		     float *sigmas,float **pSMatrix){
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
		(*pSMatrix)[i] += calcPSMEntryContrib(calcBuffer,
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


int simpleDetFunctionCalculation(int correntropyWinSize, int interval,
				 float scaleFactor, int sigWindowSize,
				 int sampleRate, int numChannels,
				 float minFreq, float maxFreq,
				 float* data, int dataLength,
				 float** detFunction)
{

	int dFLength,numWindows,bufferLength,i,startIndex;
	float *pooledSummaryMatrix, *sigmas, *centralFreq, *buffer;

	numWindows = (int)ceil((dataLength - correntropyWinSize)/
			       (float)interval) + 1;
	printf("datalen: %d, numWindows: %d\n", dataLength, numWindows);
	dFLength = numWindows - 1;

	/* for simplicity, we are just going pad the buffer with some extra 
	 * zeros at the end so that we don't need to specially treat the 
	 * function that computes correntropy. Add 2*correntropyWinSize zeros 
	 * will be sufficient (it's not exact - you could get away with adding 
	 * fewer zeros).
	 */
	bufferLength = (numWindows)*interval + 4* correntropyWinSize;
	printf("bufferLength: %d\n",bufferLength);
	buffer = malloc(sizeof(float)*bufferLength);
	for (i = numWindows * correntropyWinSize; i<bufferLength;i++){
		buffer[i] = 0;
	}

	
	pooledSummaryMatrix = malloc(sizeof(float)*numWindows);
	for (i=0;i<numWindows;i++){
		pooledSummaryMatrix[i] = 0;
	}
	sigmas = malloc(sizeof(float)*numWindows);

	centralFreq = centralFreqMapper(numChannels, minFreq, maxFreq);

	startIndex = correntropyWinSize/2;
	
	float averageTime = 0.0f;
	for (i = 0;i<numChannels;i++){
		clock_t c1 = clock();

		//printf("compute channel %d...\n", i);
		//gammatoneFilter(data, &buffer, centralFreq[i], sampleRate,dataLength);
		simpleGammatone(data, &buffer, centralFreq[i], sampleRate, dataLength);

		//printf("   gammatone %d...\n", i);
		/* compute the sigma values */
		rollSigma(startIndex, interval, scaleFactor, sigWindowSize,
			  dataLength, numWindows, buffer, &sigmas);
		//printf("   sigma %d...\n", i);
		/* compute the pooledSummaryMatrixValues */
		pSMContribution(correntropyWinSize, interval, numWindows,
				buffer, sigmas, &pooledSummaryMatrix);
		//printf("   matrix %d...\n", i);
		
		clock_t c2 = clock();
		float elapsed = ((float)(c2-c1))/CLOCKS_PER_SEC;
		printf("  %d\telapsed = %f\n", i, elapsed*1000);
		averageTime += elapsed;
	}
	printf("  average time: %f\n", (averageTime*1000) / numChannels);

	free(sigmas);
	free(buffer);
	(*detFunction) = malloc(sizeof(float)*dFLength);
	for (i = 0; i<dFLength; i++){
		(*detFunction)[i] = (pooledSummaryMatrix[i+1]
				     - pooledSummaryMatrix[i]);
	}
	free(pooledSummaryMatrix);
	return dFLength;
}


/* Defining a function to generate an array of randomly distributed values for 
   testing*/

float uniform_rand_inclusive(){
	// returns a random number with uniform distribution in [0,1]
	return ((float)rand() - 1.0)/((float)RAND_MAX-2.0);
}

void pos_rand_double_norm(float mu, float sigma, float* x, float* y){
	/* Generate 2 random numbers in normal distribution. 
	   For the random distribution, use the polar form of the Box–Muller 
	   transform described on 
	   https://en.wikipedia.org/wiki/Box%E2%80%93Muller_transform 
	   According to the page:
	   - the basic formula gives returns values from the standard normal 
	   distribution where mu = 0 (mean) & sigma = 1 (standard deviation) 
	   - to achieve a normal distribution, multiply the generated number by 
	   sigma and then add mu */
	float s, u, v;

	s = 0.0;
	while(s == 0.0 || s >= 1.0) {
		v = uniform_rand_inclusive() * 2.0 - 1.0;
		u = uniform_rand_inclusive() * 2.0 - 1.0;
		s = v*v + u*u;
	}

	*x = sigma*u * sqrtf((-2.0 * logf(s))/s) + mu;
	*y = sigma*v * sqrtf((-2.0 * logf(s))/s) + mu;
}


float* normal_distribution_array(int length, float mu, float sigma){
	float *out,t1,t2;
	int i;

	out = malloc(sizeof(float)*length);

	for (i=0;i<length;i++){
		pos_rand_double_norm(mu, sigma, &t1, &t2);
		out[i] = t1;
		i++;
		if (i<length){
			out[i] = t2;
		}
	}
	return out;
}

/*
int main(int argc, char *argv[]){
	float *array;
	int length, sampleRate;

	sampleRate = 8000;
	length = 7*sampleRate;

	//set the seed
	srand(341535264);
	array = normal_distribution_array(length, 0.0, 1.0);
	printf("Constructed Array\n");

	clock_t c1 = clock();

	float* detectionFunc = NULL;
	int detectionFuncLength = simpleDetFunctionCalculation(100, 120, powf(4./3.,0.2), 1403,
				     sampleRate, 64,
				     80, 4000,
				     array, length,
				     &detectionFunc);

	clock_t c2 = clock();

	float elapsed = ((float)(c2-c1))/CLOCKS_PER_SEC;
	printf("elapsed = %e\n",(elapsed/1000.f));
	
	free(array);
	free(detectionFunc);
	return 1;
}
*/
