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

void calcSigma(int startIndex, int interval, float scaleFactor,
	       int sigWindowSize, int dataLength, int numWindows,
	       float *buffer, float **sigmas){
	struct windowIndexer* wInd;
	int i,j,winStart,winStop;
	double s1,s2,temp, nsamples;
	float std;
	// sigWindowSize has units of intervals. We need it to have units of
	// samples
	wInd = windowIndexerNew(1, 1, sigWindowSize*interval, interval,
				startIndex, dataLength);

	for (i=0;i<numWindows;i++){
		winStart = wIndGetStart(wInd);
		winStop = wIndGetStop(wInd);
		//printf("winStart: %d, winStop: %d\n", winStart, winStop);
		nsamples = (double)(winStop - winStart);
		s1 = 0;
		s2 = 0;
		for (j=winStart;j<winStop;j++){
			temp = (double)buffer[j];
			s1 += temp;
			s2 += temp*temp;
		}
		std =(float)sqrt((nsamples*s2-(s1*s1))/(nsamples*(nsamples-1)));
		(*sigmas)[i] = (scaleFactor*std/powf((float)nsamples,0.2));
	}
}

#define M_1_SQRT2PI 0.3989422804
static inline float calcPSMEntryContrib(float* x, int start, int window_size,
					float sigma)
{
	int i,j,off1,off2;
	float out,temp,denom;
	out=0;
	denom = M_SQRT1_2/sigma;
	off1=start;
	for (i=1;i<=window_size;i++){
		x[i]*=denom;
	}

	off1=start;
	for (i=1;i<=window_size;i++){
		off1++;
		off2 = off1;
		for (j=i; j<=window_size;j++){
			off2++;
			temp = x[off1]- x[off2]; 
			out+=expf(-temp * temp);
		}
	}
	out *= M_1_SQRT2PI / sigma;
	return out;
}

void pSMContribution(int correntropyWinSize, int interval,
		     int numWindows, float *buffer,
		     float *sigmas,float **pSMatrix){
	int i,start;
	start = 0;
	
	for (i=0;i<numWindows;i++){
		// not sure if the following function will work correctly:
		(*pSMatrix)[i] = calcPSMEntryContrib(buffer, start,
						     correntropyWinSize,
						     sigmas[i]);
		start+=interval;
	}
}


int simpleDetFunctionCalculation(int correntropyWinSize, int interval,
				 float scaleFactor, int sigWindowSize,
				 int sampleRate, int numChannels,
				 float minFreq, float maxFreq,
				 float* data, int dataLength,
				 float** detFunction){

	int dFLength,numWindows,bufferLength,i,startIndex;
	float *pooledSummaryMatrix, *sigmas, *centralFreq, *buffer;

	numWindows = (int)ceil((dataLength - correntropyWinSize)/
			       (float)interval) + 1;
	dFLength = numWindows - 1;

	/* for simplicity, we are just going pad the buffer with some extra 
	 * zeros at the end so that we don't need to specially treat the 
	 * function that computes correntropy. Add 2*correntropyWinSize zeros 
	 * will be sufficient (it's not exact - you could get away with adding 
	 * fewer zeros).
	 */
	bufferLength = (numWindows + 2)*correntropyWinSize;
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
	
	for (i = 0;i<numChannels;i++){
		gammatoneFilter(data, &buffer, centralFreq[i], sampleRate,
				dataLength);
		/* compute the sigma values */
		calcSigma(startIndex, interval, scaleFactor, sigWindowSize,
			  dataLength, numWindows, buffer, &sigmas);
		/* compute the pooledSummaryMatrixValues */
		pSMContribution(correntropyWinSize, interval, numWindows,
				buffer, sigmas, &pooledSummaryMatrix);
	}

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


int main(int argc, char *argv[]){
	// set the seed
	float *array,*result, center_freq, elapsed;
	int length, sampleRate,i,num;
	clock_t c1,c2;

	sampleRate = 11025;
	length = 7*sampleRate;
	//length = 1403*55;
	center_freq = 38610+274;
	num = 10000;
	num = 1;
	result = 1;

	srand(341535264);
	array = normal_distribution_array(length, 0.0, 1.0);
	printf("Constructed Array\n");
	c1 = clock();
	simpleDetFunctionCalculation(137, 55, powf(4./3.,0.2), 1403,
				     sampleRate, 64,
				     80, 4000,
				     array, length,
				     &result);
	c2 = clock();

	elapsed = ((float)(c2-c1))/CLOCKS_PER_SEC;
	
	printf("elapsed = %e\n",(elapsed/num));
	free(array);
	free(result);
	return 1;
}
