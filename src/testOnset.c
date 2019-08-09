#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <float.h>

#include "testOnset.h"

//wmin set to 4 (20ms)
//wmax set to 500 (2.5s)
//based on correntropy hopsize h being 5ms

//Aw = Kernel of len w
//Ww = window of detection function W of len w
//Fitness(Aw, Ww) = (Aw - Ww)^2 * w^(-1)

//Aw = z/(1 + a - |z|), a = 0.15, z is split into w evenly spaced values on range  -1+10^-5 <= z <= 1-10^-5
float MINZ = -0.99999f;
float MAXZ = 0.99999f;
float DOUBLEMAXZ = 1.99998f;

float calcZ(int index, int max)
{
	return ((DOUBLEMAXZ * index) / (max-1)) + MINZ;
}

float** GenKernels(int minK, int maxK)
{
	int numKernels = maxK - minK + 1;
	int i, j, kernelLen;
	float z;
	if(numKernels <= 0){
		return NULL;
	}
	float** kernels = malloc(numKernels * sizeof(float*));
	for(i = 0, kernelLen = minK; i < numKernels; i++, kernelLen++)
	{
		kernels[i] = malloc(kernelLen * sizeof(float));
		for(j = 0; j < kernelLen; ++j){
			z = calcZ(j, kernelLen);
			kernels[i][j] = z/(1.15f - fabs(z));
		}
	}
	return kernels;
}

void freeKernels(float** kernels, int numKernels){
	for(int i = 0; i < numKernels; ++i){
		free(kernels[i]);
	}
	free(kernels);
}

float FitnessOnset(float* kernel, float* window, int start, int len)
{	
	//In the paper, the equation for the onset fitness (Fig 6) had kernel[i] positive, and the offset had kernel[i] negative.
	//but the related graph of the fitness functions (Fig 4) showed their signs flipped.
	//after empirical testing, the sign flipped kernel in Fig 4 seems correct, and Fig 6 seems incorrect
	float sum = 0.0f;
	float* winptr = &window[start];
	for(int i = 0; i < len; ++i, ++winptr){
		sum += (-kernel[i] - (*winptr))*(-kernel[i] - (*winptr));
	}
	return sum/len;
}

float FitnessOffset(float* kernel, float* window, int start, int len)
{
	//only difference from FitnessOnset is kernel[i] is not negated
	float sum = 0.0f;
	float* winptr = &window[start];
	for(int i = 0; i < len; ++i, ++winptr){
		sum += (kernel[i] - (*winptr))*(kernel[i] - (*winptr));
	}
	return sum/len;
}

//populates transients, which will hold indices of onsets and offsets in detection_func
int detectTransients(intList* transients, float* detection_func, int len){
	int minkernel = 4;
	int maxkernel = 1500;

	printf("detection with len %d, minK %d, maxK %d\n", len, minkernel, maxkernel);

	float** Kernels = GenKernels(minkernel, maxkernel);
	int numKernels = maxkernel - minkernel + 1;

	printf("kernel made\n");

	int detect_index = 0;
	int tmpMax = 0;
	int lastpossibleStart = len-8; //last possible starting index bc notes are at least 8 indices for onset+offset.
	int lastpossibleOnset = len-4; //last possible onset index bc offset requires at least 4 indices.

	float bestFitness;
	int bestInd;

	float curFitness;
	int i;

	while(detect_index < lastpossibleStart){

		//printf("start while index - %d\n", detect_index);

		bestFitness = FLT_MAX;
		bestInd = 0;
		tmpMax = maxkernel < (len - detect_index) ? maxkernel : (len - detect_index);
		for(i = minkernel; i < tmpMax; ++i){ 
			curFitness = FitnessOnset(Kernels[i - minkernel], detection_func, detect_index, i);
			if(curFitness < bestFitness){
				bestFitness = curFitness;
				bestInd = i;
			}
		}
		detect_index += bestInd;
		if(detect_index >= lastpossibleOnset){
			//onset detected too close to end for a corresponding offset, so we skip the onset and end.
			//printf("onset detected too close to end, break\n");
			break;
		}

		//printf("    ONSET   FITNESS:  %f  AT INDEX:  %d   AT TIME:  %f\n", bestFitness, bestInd, detect_index/200.0f);
		if(intListAppend(transients, detect_index) != 1){
			printf("Resizing transients failed. Exitting.\n");
			freeKernels(Kernels, numKernels);
			return -1;
		}

		bestFitness = FLT_MAX;
		bestInd = 0;
		tmpMax = maxkernel < (len - detect_index) ? maxkernel : (len - detect_index);
		for(i = minkernel; i < tmpMax; ++i){
			curFitness = FitnessOffset(Kernels[i - minkernel], detection_func, detect_index, i);
			if(curFitness < bestFitness){
				bestFitness = curFitness;
				bestInd = i;
			}
		}
		detect_index += bestInd;

		//printf("    OFFSET   FITNESS:  %f  AT INDEX:  %d   AT TIME:  %f\n", bestFitness, bestInd, detect_index/200.0f);
		if(intListAppend(transients, detect_index) != 1){
			printf("Resizing transients failed. Exitting.\n");
			freeKernels(Kernels, numKernels);
			return -1;
		}
		// if at end of activity range, jump detect_index forward to
		// start of next range
	}

	freeKernels(Kernels, numKernels);

	// the transient detection algorithm, by its design, will (almost)
	// always have an extra false positive note at the end. we only go up
	// to transients->length-2 to remove this note
	transients->length -= 2;
	if(transients->length <= 0){
		transients->length = 0;
		// no transients found, do not attempt to shrink here. Unable
		// to realloc array to size 0
		return 0;
	}

	if (intListShrink(transients)!=1){
		printf("Resizing onsets failed. Exitting.\n");
		// intList was preallocated, we intentionally won't free it
		return -1;
	}

	return transients->length;
}
